import {
  ForbiddenException,
  Injectable,
  NotFoundException,
} from '@nestjs/common';
import { GatewayService } from '../gateway/gateway.service';
import { PrismaService } from '../prisma/prisma.service';
import { sanitizeMessageContent, sanitizePlainText } from '../common/sanitize';
import { CreateDmMessageDto } from './dto/create-dm-message.dto';
import { CreateThreadDto } from './dto/create-thread.dto';
import { ListDmMessagesQueryDto } from './dto/list-dm-messages.dto';

@Injectable()
export class DmService {
  constructor(
    private readonly prisma: PrismaService,
    private readonly gateway: GatewayService,
  ) {}

  async listThreads(userId: string) {
    const threads = await this.prisma.dMThread.findMany({
      where: {
        members: {
          some: { userId },
        },
      },
      include: {
        members: {
          include: {
            user: {
              select: {
                id: true,
                displayName: true,
                avatarUrl: true,
                presence: true,
              },
            },
          },
        },
        messages: {
          take: 1,
          orderBy: { createdAt: 'desc' },
          select: { id: true, content: true, createdAt: true, authorId: true },
        },
      },
      orderBy: { updatedAt: 'desc' },
    });

    return threads.map((thread) => ({
      id: thread.id,
      name: thread.name,
      isGroup: thread.isGroup,
      participants: thread.members.map((member) => member.user),
      lastMessage: thread.messages[0] ?? null,
    }));
  }

  async createThread(userId: string, dto: CreateThreadDto) {
    const uniqueParticipants = [...new Set([userId, ...dto.participantIds])];

    if (uniqueParticipants.length < 2) {
      throw new ForbiddenException('Thread must contain at least two users');
    }

    if (uniqueParticipants.length === 2) {
      const existing = await this.prisma.dMThread.findFirst({
        where: {
          isGroup: false,
          members: {
            every: {
              userId: { in: uniqueParticipants },
            },
          },
        },
        include: {
          members: {
            include: {
              user: {
                select: { id: true, displayName: true, avatarUrl: true, presence: true },
              },
            },
          },
        },
      });

      if (existing) {
        return {
          id: existing.id,
          name: existing.name,
          isGroup: existing.isGroup,
          participants: existing.members.map((member) => member.user),
        };
      }
    }

    const thread = await this.prisma.dMThread.create({
      data: {
        ownerId: userId,
        isGroup: uniqueParticipants.length > 2,
        name: dto.name ? sanitizePlainText(dto.name) : null,
        members: {
          createMany: {
            data: uniqueParticipants.map((participantId) => ({ userId: participantId })),
          },
        },
      },
      include: {
        members: {
          include: {
            user: {
              select: { id: true, displayName: true, avatarUrl: true, presence: true },
            },
          },
        },
      },
    });

    this.gateway.broadcast('dm_thread_create', {
      threadId: thread.id,
      participantIds: uniqueParticipants,
    });

    return {
      id: thread.id,
      name: thread.name,
      isGroup: thread.isGroup,
      participants: thread.members.map((member) => member.user),
    };
  }

  async listMessages(userId: string, threadId: string, query: ListDmMessagesQueryDto) {
    await this.ensureThreadMember(userId, threadId);
    const limit = query.limit ?? 50;

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
        dmThreadId: threadId,
        deletedAt: null,
        ...cursorFilter,
      },
      include: {
        author: {
          select: { id: true, displayName: true, avatarUrl: true, presence: true },
        },
        attachments: true,
        reactions: true,
      },
      orderBy: { createdAt: 'desc' },
      take: limit + 1,
    });

    const hasMore = messages.length > limit;
    const items = hasMore ? messages.slice(0, limit) : messages;

    return {
      items,
      nextCursor: hasMore ? items[items.length - 1].id : null,
    };
  }

  async createMessage(userId: string, threadId: string, dto: CreateDmMessageDto) {
    await this.ensureThreadMember(userId, threadId);

    const message = await this.prisma.message.create({
      data: {
        dmThreadId: threadId,
        authorId: userId,
        content: sanitizeMessageContent(dto.content),
      },
      include: {
        author: {
          select: { id: true, displayName: true, avatarUrl: true, presence: true },
        },
        attachments: true,
        reactions: true,
      },
    });

    this.gateway.emitToDmThread(threadId, 'dm_message_create', message);

    return message;
  }

  private async ensureThreadMember(userId: string, threadId: string) {
    const member = await this.prisma.dMThreadMember.findUnique({
      where: {
        threadId_userId: {
          threadId,
          userId,
        },
      },
    });

    if (!member) {
      throw new NotFoundException('Thread not found');
    }
  }
}
