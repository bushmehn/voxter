import {
  BadRequestException,
  ForbiddenException,
  Injectable,
  NotFoundException,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { Message } from '@prisma/client';
import { randomUUID } from 'crypto';
import { Client as MinioClient } from 'minio';
import { PermissionBits } from '../common/constants/permissions';
import { sanitizeMessageContent } from '../common/sanitize';
import { applyOverwrite, hasPermission } from '../common/utils/permission-resolver';
import { GatewayService } from '../gateway/gateway.service';
import { GuildsService } from '../guilds/guilds.service';
import { PrismaService } from '../prisma/prisma.service';
import { CreateMessageDto } from './dto/create-message.dto';
import { ListMessagesQueryDto } from './dto/list-messages-query.dto';
import { UpdateMessageDto } from './dto/update-message.dto';
import { UploadAttachmentDto } from './dto/upload-attachment.dto';
import { promises as fs } from 'fs';
import { join } from 'path';

@Injectable()
export class MessagesService {
  private readonly storageDriver: 'local' | 'minio';
  private readonly uploadsDir: string;
  private readonly minioBucket: string;
  private readonly minioClient?: MinioClient;

  constructor(
    private readonly prisma: PrismaService,
    private readonly guildsService: GuildsService,
    private readonly gateway: GatewayService,
    private readonly config: ConfigService,
  ) {
    this.storageDriver = this.config.get<'local' | 'minio'>('STORAGE_DRIVER', 'local');
    this.uploadsDir = this.config.get<string>('UPLOADS_DIR', 'uploads');
    this.minioBucket = this.config.get<string>('MINIO_BUCKET', 'voxter');

    if (this.storageDriver === 'minio') {
      this.minioClient = new MinioClient({
        endPoint: this.config.get<string>('MINIO_ENDPOINT', 'localhost'),
        port: Number(this.config.get<string>('MINIO_PORT', '9000')),
        useSSL: this.config.get<string>('MINIO_USE_SSL', 'false') === 'true',
        accessKey: this.config.get<string>('MINIO_ACCESS_KEY', ''),
        secretKey: this.config.get<string>('MINIO_SECRET_KEY', ''),
      });
    }
  }

  async listChannelMessages(
    userId: string,
    channelId: string,
    query: ListMessagesQueryDto,
  ) {
    await this.ensureChannelPermission(userId, channelId, PermissionBits.ViewChannel);

    const limit = query.limit ?? 50;
    if (query.search?.trim()) {
      const rows = await this.prisma.$queryRaw<Array<{ id: string }>>`
        SELECT m."id"
        FROM "Message" m
        WHERE m."channelId" = ${channelId}
          AND m."deletedAt" IS NULL
          AND m."searchVector" @@ plainto_tsquery('simple', ${query.search.trim()})
        ORDER BY m."createdAt" DESC
        LIMIT ${limit}
      `;

      const ids = rows.map((row) => row.id);
      const messages = await this.prisma.message.findMany({
        where: { id: { in: ids } },
        include: {
          author: { select: { id: true, displayName: true, avatarUrl: true, presence: true } },
          attachments: true,
          reactions: true,
        },
        orderBy: { createdAt: 'desc' },
      });

      return {
        items: messages.map((message) => this.formatMessage(message)),
        nextCursor: null,
      };
    }

    let cursorFilter = {};
    if (query.cursor) {
      const cursorMessage = await this.prisma.message.findUnique({ where: { id: query.cursor } });
      if (cursorMessage) {
        cursorFilter = {
          createdAt: {
            lt: cursorMessage.createdAt,
          },
        };
      }
    }

    const messages = await this.prisma.message.findMany({
      where: {
        channelId,
        deletedAt: null,
        ...cursorFilter,
      },
      include: {
        author: { select: { id: true, displayName: true, avatarUrl: true, presence: true } },
        attachments: true,
        reactions: true,
      },
      orderBy: { createdAt: 'desc' },
      take: limit + 1,
    });

    const hasMore = messages.length > limit;
    const items = hasMore ? messages.slice(0, limit) : messages;

    return {
      items: items.map((message) => this.formatMessage(message)),
      nextCursor: hasMore ? items[items.length - 1].id : null,
    };
  }

  async createChannelMessage(userId: string, channelId: string, dto: CreateMessageDto) {
    await this.ensureChannelPermission(userId, channelId, PermissionBits.SendMessages);
    const content = sanitizeMessageContent(dto.content ?? '');
    if (!content) {
      throw new BadRequestException('Message content cannot be empty');
    }

    const message = await this.prisma.message.create({
      data: {
        channelId,
        authorId: userId,
        content,
      },
      include: {
        author: { select: { id: true, displayName: true, avatarUrl: true, presence: true } },
        attachments: true,
        reactions: true,
      },
    });

    const payload = this.formatMessage(message);
    this.gateway.emitToChannel(channelId, 'message_create', payload);
    return payload;
  }

  async updateChannelMessage(
    userId: string,
    channelId: string,
    messageId: string,
    dto: UpdateMessageDto,
  ) {
    const message = await this.prisma.message.findUnique({
      where: { id: messageId },
      include: { channel: true },
    });

    if (!message || message.deletedAt || message.channelId !== channelId) {
      throw new NotFoundException('Message not found');
    }

    const permissions = await this.resolvePermissions(userId, channelId);
    const canManage = hasPermission(permissions, PermissionBits.ManageMessages);
    if (message.authorId !== userId && !canManage) {
      throw new ForbiddenException('No rights to edit this message');
    }

    const updated = await this.prisma.message.update({
      where: { id: message.id },
      data: {
        content: sanitizeMessageContent(dto.content),
        editedAt: new Date(),
      },
      include: {
        author: { select: { id: true, displayName: true, avatarUrl: true, presence: true } },
        attachments: true,
        reactions: true,
      },
    });

    const payload = this.formatMessage(updated);
    this.gateway.emitToChannel(channelId, 'message_update', payload);
    return payload;
  }

  async deleteChannelMessage(userId: string, channelId: string, messageId: string) {
    const message = await this.prisma.message.findUnique({
      where: { id: messageId },
    });

    if (!message || message.deletedAt || message.channelId !== channelId) {
      throw new NotFoundException('Message not found');
    }

    const permissions = await this.resolvePermissions(userId, channelId);
    const canManage = hasPermission(permissions, PermissionBits.ManageMessages);
    if (message.authorId !== userId && !canManage) {
      throw new ForbiddenException('No rights to delete this message');
    }

    await this.prisma.message.update({
      where: { id: message.id },
      data: { deletedAt: new Date() },
    });

    this.gateway.emitToChannel(channelId, 'message_delete', {
      channelId,
      messageId,
    });

    return { ok: true };
  }

  async toggleReaction(
    userId: string,
    channelId: string,
    messageId: string,
    emoji: string,
  ) {
    await this.ensureChannelPermission(userId, channelId, PermissionBits.ViewChannel);

    const existing = await this.prisma.reaction.findUnique({
      where: {
        messageId_userId_emoji: {
          messageId,
          userId,
          emoji,
        },
      },
    });

    if (existing) {
      await this.prisma.reaction.delete({ where: { id: existing.id } });
      this.gateway.emitToChannel(channelId, 'message_reaction_remove', {
        messageId,
        userId,
        emoji,
      });
      return { action: 'removed' };
    }

    await this.prisma.reaction.create({
      data: {
        messageId,
        userId,
        emoji,
      },
    });

    this.gateway.emitToChannel(channelId, 'message_reaction_add', {
      messageId,
      userId,
      emoji,
    });

    return { action: 'added' };
  }

  async sendTyping(userId: string, channelId: string) {
    await this.ensureChannelPermission(userId, channelId, PermissionBits.ViewChannel);
    this.gateway.emitToChannel(channelId, 'typing', {
      channelId,
      userId,
    });

    return { ok: true };
  }

  async createAttachmentMessage(
    userId: string,
    channelId: string,
    file: Express.Multer.File,
    dto: UploadAttachmentDto,
  ) {
    await this.ensureChannelPermission(userId, channelId, PermissionBits.SendMessages);

    if (!file) {
      throw new BadRequestException('Attachment file is required');
    }

    const objectKey = `${randomUUID()}-${file.originalname}`;
    const uploadResult = await this.uploadFile(objectKey, file);

    const message = await this.prisma.message.create({
      data: {
        channelId,
        authorId: userId,
        content: sanitizeMessageContent(dto.content ?? file.originalname),
        attachments: {
          create: {
            fileName: file.originalname,
            mimeType: file.mimetype,
            size: file.size,
            url: uploadResult.url,
          },
        },
      },
      include: {
        author: { select: { id: true, displayName: true, avatarUrl: true, presence: true } },
        attachments: true,
        reactions: true,
      },
    });

    const payload = this.formatMessage(message);
    this.gateway.emitToChannel(channelId, 'message_create', payload);
    return payload;
  }

  private async uploadFile(objectKey: string, file: Express.Multer.File): Promise<{ url: string }> {
    if (this.storageDriver === 'minio' && this.minioClient) {
      const bucketExists = await this.minioClient.bucketExists(this.minioBucket);
      if (!bucketExists) {
        await this.minioClient.makeBucket(this.minioBucket, 'us-east-1');
      }

      await this.minioClient.putObject(
        this.minioBucket,
        objectKey,
        file.buffer,
        file.size,
        {
          'Content-Type': file.mimetype,
        },
      );

      const url = `/minio/${this.minioBucket}/${objectKey}`;
      return { url };
    }

    await fs.mkdir(this.uploadsDir, { recursive: true });
    const targetPath = join(this.uploadsDir, objectKey);
    await fs.writeFile(targetPath, file.buffer);
    return { url: `/uploads/${objectKey}` };
  }

  private async ensureChannelPermission(userId: string, channelId: string, permission: bigint) {
    const computed = await this.resolvePermissions(userId, channelId);
    if (!hasPermission(computed, permission)) {
      throw new ForbiddenException('Missing channel permissions');
    }
  }

  private async resolvePermissions(userId: string, channelId: string): Promise<bigint> {
    const channel = await this.prisma.channel.findUnique({
      where: { id: channelId },
      include: { overwrites: true },
    });

    if (!channel) {
      throw new NotFoundException('Channel not found');
    }

    const guildPermissions = await this.guildsService.resolveGuildPermissions(userId, channel.guildId);

    const userOverwrite = channel.overwrites.find((overwrite) => overwrite.userId === userId);

    const roles = await this.prisma.guildMemberRole.findMany({
      where: {
        guildId: channel.guildId,
        userId,
      },
      select: { roleId: true },
    });

    const roleOverwrite = channel.overwrites.find(
      (overwrite) => overwrite.roleId && roles.some((role) => role.roleId === overwrite.roleId),
    );

    const withRole = applyOverwrite(guildPermissions, roleOverwrite ?? undefined);
    return applyOverwrite(withRole, userOverwrite ?? undefined);
  }

  private formatMessage(message: Message & Record<string, unknown>) {
    return {
      id: message.id,
      channelId: message.channelId,
      dmThreadId: message.dmThreadId,
      authorId: message.authorId,
      content: message.content,
      editedAt: message.editedAt,
      deletedAt: message.deletedAt,
      createdAt: message.createdAt,
      updatedAt: message.updatedAt,
      author: message.author,
      attachments: message.attachments,
      reactions: message.reactions,
    };
  }
}
