import { Injectable } from '@nestjs/common';
import { PresenceStatus } from '@prisma/client';
import { PermissionBits } from '../common/constants/permissions';
import { GatewayService } from '../gateway/gateway.service';
import { PrismaService } from '../prisma/prisma.service';
import { RedisService } from '../redis/redis.service';

@Injectable()
export class PresenceService {
  constructor(
    private readonly prisma: PrismaService,
    private readonly redis: RedisService,
    private readonly gateway: GatewayService,
  ) {}

  async updateStatus(userId: string, status: PresenceStatus) {
    const user = await this.prisma.user.update({
      where: { id: userId },
      data: { presence: status },
      select: { id: true, presence: true },
    });

    await this.redis.setPresence(userId, status);

    this.gateway.broadcast('presence_update', {
      userId,
      status,
    });

    return user;
  }

  async listGuildPresence(userId: string, guildId: string) {
    const membership = await this.prisma.guildMember.findUnique({
      where: {
        guildId_userId: {
          guildId,
          userId,
        },
      },
    });

    if (!membership) {
      return [];
    }

    const guild = await this.prisma.guild.findUnique({
      where: { id: guildId },
      select: { ownerId: true },
    });

    const members = await this.prisma.guildMember.findMany({
      where: { guildId },
      include: {
        user: {
          select: {
            id: true,
            displayName: true,
            avatarUrl: true,
            presence: true,
          },
        },
        roleLinks: {
          include: {
            role: {
              select: {
                permissions: true,
              },
            },
          },
        },
      },
    });

    return members.map((member) => {
      const rolePermissions = member.roleLinks.reduce(
        (acc, link) => acc | link.role.permissions,
        0n,
      );
      const hasManageGuild =
        (rolePermissions & PermissionBits.ManageGuild) === PermissionBits.ManageGuild;
      const isOwner = member.userId === guild?.ownerId;

      return {
        ...member.user,
        isOwner,
        isAdmin: isOwner || hasManageGuild,
      };
    });
  }
}
