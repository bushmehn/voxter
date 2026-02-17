import {
  ForbiddenException,
  Injectable,
  NotFoundException,
} from '@nestjs/common';
import { ChannelType } from '@prisma/client';
import { randomBytes } from 'crypto';
import { PermissionBits, PermissionPreset } from '../common/constants/permissions';
import { combinePermissions, hasPermission } from '../common/utils/permission-resolver';
import { GatewayService } from '../gateway/gateway.service';
import { PrismaService } from '../prisma/prisma.service';
import { CreateChannelDto } from './dto/create-channel.dto';
import { CreateGuildDto } from './dto/create-guild.dto';
import { CreateInviteDto } from './dto/create-invite.dto';
import { CreateRoleDto } from './dto/create-role.dto';
import { SetOverwriteDto } from './dto/set-overwrite.dto';
import { UpdateRoleDto } from './dto/update-role.dto';

@Injectable()
export class GuildsService {
  constructor(
    private readonly prisma: PrismaService,
    private readonly gateway: GatewayService,
  ) {}

  async listMyGuilds(userId: string) {
    const memberships = await this.prisma.guildMember.findMany({
      where: { userId },
      include: {
        guild: {
          include: {
            channels: true,
          },
        },
      },
      orderBy: { joinedAt: 'asc' },
    });

    return memberships.map((entry) => ({
      id: entry.guild.id,
      name: entry.guild.name,
      iconUrl: entry.guild.iconUrl,
      ownerId: entry.guild.ownerId,
      channels: entry.guild.channels.map((channel) => ({
        id: channel.id,
        name: channel.name,
        type: channel.type,
        position: channel.position,
      })),
    }));
  }

  async createGuild(userId: string, dto: CreateGuildDto) {
    const guild = await this.prisma.$transaction(async (tx) => {
      const createdGuild = await tx.guild.create({
        data: {
          name: dto.name.trim(),
          ownerId: userId,
        },
      });

      const everyoneRole = await tx.role.create({
        data: {
          guildId: createdGuild.id,
          name: '@everyone',
          isDefault: true,
          permissions: PermissionPreset.Member,
          position: 0,
        },
      });

      const ownerRole = await tx.role.create({
        data: {
          guildId: createdGuild.id,
          name: 'Owner',
          position: 10,
          permissions: PermissionPreset.Admin,
        },
      });

      await tx.guildMember.create({
        data: {
          guildId: createdGuild.id,
          userId,
          roleLinks: {
            create: [{ roleId: everyoneRole.id }, { roleId: ownerRole.id }],
          },
        },
      });

      await tx.channel.createMany({
        data: [
          {
            guildId: createdGuild.id,
            name: 'general',
            type: ChannelType.TEXT,
            position: 0,
          },
          {
            guildId: createdGuild.id,
            name: 'voice',
            type: ChannelType.VOICE,
            position: 1,
          },
        ],
      });

      return createdGuild;
    });

    return guild;
  }

  async listChannels(userId: string, guildId: string) {
    await this.ensureMember(userId, guildId);

    const channels = await this.prisma.channel.findMany({
      where: { guildId },
      orderBy: { position: 'asc' },
    });

    return channels;
  }

  async createChannel(userId: string, guildId: string, dto: CreateChannelDto) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageChannels)) {
      throw new ForbiddenException('Missing MANAGE_CHANNELS permission');
    }

    const channel = await this.prisma.channel.create({
      data: {
        guildId,
        name: dto.name.trim(),
        type: dto.type,
        position: dto.position ?? 0,
      },
    });

    this.gateway.emitToGuild(guildId, 'channel_create', {
      guildId,
      channel,
    });

    return channel;
  }

  async createInvite(userId: string, guildId: string, dto: CreateInviteDto) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageGuild)) {
      throw new ForbiddenException('Missing MANAGE_GUILD permission');
    }

    const code = randomBytes(4).toString('hex');
    const invite = await this.prisma.invite.create({
      data: {
        code,
        guildId,
        creatorId: userId,
        maxUses: dto.maxUses,
        expiresAt: dto.expiresAt ? new Date(dto.expiresAt) : null,
      },
    });

    return {
      code: invite.code,
      guildId,
      expiresAt: invite.expiresAt,
      maxUses: invite.maxUses,
      uses: invite.uses,
    };
  }

  async joinByInvite(userId: string, code: string) {
    const invite = await this.prisma.invite.findUnique({
      where: { code },
      include: { guild: true },
    });

    if (!invite || invite.revoked) {
      throw new NotFoundException('Invite not found');
    }

    if (invite.expiresAt && invite.expiresAt < new Date()) {
      throw new ForbiddenException('Invite expired');
    }

    if (invite.maxUses && invite.uses >= invite.maxUses) {
      throw new ForbiddenException('Invite max uses reached');
    }

    const membership = await this.prisma.guildMember.findUnique({
      where: {
        guildId_userId: {
          guildId: invite.guildId,
          userId,
        },
      },
    });

    if (!membership) {
      const defaultRole = await this.prisma.role.findFirst({
        where: { guildId: invite.guildId, isDefault: true },
      });

      await this.prisma.guildMember.create({
        data: {
          guildId: invite.guildId,
          userId,
          roleLinks: defaultRole
            ? {
                create: [{ roleId: defaultRole.id }],
              }
            : undefined,
        },
      });
    }

    await this.prisma.invite.update({
      where: { id: invite.id },
      data: { uses: { increment: 1 } },
    });

    this.gateway.emitToGuild(invite.guildId, 'guild_member_join', {
      guildId: invite.guildId,
      userId,
    });

    return {
      guildId: invite.guildId,
      guildName: invite.guild.name,
    };
  }

  async listRoles(userId: string, guildId: string) {
    await this.ensureMember(userId, guildId);
    const roles = await this.prisma.role.findMany({
      where: { guildId },
      orderBy: [{ position: 'desc' }, { createdAt: 'asc' }],
    });

    return roles.map((role) => ({
      ...role,
      permissions: role.permissions.toString(),
    }));
  }

  async createRole(userId: string, guildId: string, dto: CreateRoleDto) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageGuild)) {
      throw new ForbiddenException('Missing MANAGE_GUILD permission');
    }

    const role = await this.prisma.role.create({
      data: {
        guildId,
        name: dto.name,
        color: dto.color,
        position: dto.position ?? 1,
        permissions: BigInt(dto.permissions),
      },
    });

    return {
      ...role,
      permissions: role.permissions.toString(),
    };
  }

  async updateRole(
    userId: string,
    guildId: string,
    roleId: string,
    dto: UpdateRoleDto,
  ) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageGuild)) {
      throw new ForbiddenException('Missing MANAGE_GUILD permission');
    }

    const existingRole = await this.prisma.role.findFirst({
      where: { id: roleId, guildId },
    });
    if (!existingRole) {
      throw new NotFoundException('Role not found');
    }

    const role = await this.prisma.role.update({
      where: { id: roleId },
      data: {
        name: dto.name,
        color: dto.color,
        position: dto.position,
        permissions: dto.permissions ? BigInt(dto.permissions) : undefined,
      },
    });

    return {
      ...role,
      permissions: role.permissions.toString(),
    };
  }

  async assignRole(
    userId: string,
    guildId: string,
    memberId: string,
    roleId: string,
  ) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageGuild)) {
      throw new ForbiddenException('Missing MANAGE_GUILD permission');
    }

    await this.ensureMember(memberId, guildId);

    await this.prisma.guildMemberRole.upsert({
      where: {
        guildId_userId_roleId: {
          guildId,
          userId: memberId,
          roleId,
        },
      },
      create: {
        guildId,
        userId: memberId,
        roleId,
      },
      update: {},
    });

    return { ok: true };
  }

  async setOverwrite(
    userId: string,
    guildId: string,
    channelId: string,
    dto: SetOverwriteDto,
  ) {
    const permissions = await this.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.ManageChannels)) {
      throw new ForbiddenException('Missing MANAGE_CHANNELS permission');
    }

    const overwrite = await this.prisma.channelPermissionOverwrite.upsert({
      where: {
        id: `${channelId}:${dto.subjectType}:${dto.subjectId}`,
      },
      create: {
        id: `${channelId}:${dto.subjectType}:${dto.subjectId}`,
        channelId,
        roleId: dto.subjectType === 'role' ? dto.subjectId : null,
        userId: dto.subjectType === 'user' ? dto.subjectId : null,
        allow: BigInt(dto.allow),
        deny: BigInt(dto.deny),
      },
      update: {
        allow: BigInt(dto.allow),
        deny: BigInt(dto.deny),
      },
    });

    return {
      ...overwrite,
      allow: overwrite.allow.toString(),
      deny: overwrite.deny.toString(),
    };
  }

  async resolveGuildPermissions(userId: string, guildId: string): Promise<bigint> {
    const member = await this.prisma.guildMember.findUnique({
      where: {
        guildId_userId: {
          guildId,
          userId,
        },
      },
      include: {
        roleLinks: {
          include: {
            role: true,
          },
        },
      },
    });

    if (!member) {
      throw new ForbiddenException('User is not a guild member');
    }

    const rolePermissions = member.roleLinks.map((link) => link.role.permissions);
    return combinePermissions(rolePermissions);
  }

  async ensureMember(userId: string, guildId: string) {
    const member = await this.prisma.guildMember.findUnique({
      where: {
        guildId_userId: {
          guildId,
          userId,
        },
      },
    });

    if (!member) {
      throw new ForbiddenException('User is not a guild member');
    }

    return member;
  }
}
