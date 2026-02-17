import {
  BadRequestException,
  ForbiddenException,
  Injectable,
  NotFoundException,
  ServiceUnavailableException,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { ChannelType } from '@prisma/client';
import { AccessToken, RoomServiceClient } from 'livekit-server-sdk';
import { PermissionBits } from '../common/constants/permissions';
import { hasPermission } from '../common/utils/permission-resolver';
import { GatewayService } from '../gateway/gateway.service';
import { GuildsService } from '../guilds/guilds.service';
import { PrismaService } from '../prisma/prisma.service';
import { JoinVoiceDto } from './dto/join-voice.dto';
import { UpdateVoiceStateDto } from './dto/update-voice.dto';

@Injectable()
export class VoiceService {
  private readonly livekitWsUrl: string;
  private readonly livekitRestUrl: string;
  private readonly livekitApiKey: string;
  private readonly livekitApiSecret: string;
  private readonly livekitRoomPrefix: string;
  private readonly roomServiceClient: RoomServiceClient | null;

  constructor(
    private readonly prisma: PrismaService,
    private readonly guildsService: GuildsService,
    private readonly gateway: GatewayService,
    private readonly config: ConfigService,
  ) {
    this.livekitWsUrl = this.config.get<string>('LIVEKIT_WS_URL') ?? 'ws://localhost:7880';
    this.livekitRestUrl =
      this.config.get<string>('LIVEKIT_REST_URL') ??
      this.livekitWsUrl.replace(/^ws:/, 'http:').replace(/^wss:/, 'https:');
    this.livekitApiKey = this.config.get<string>('LIVEKIT_API_KEY') ?? '';
    this.livekitApiSecret = this.config.get<string>('LIVEKIT_API_SECRET') ?? '';
    this.livekitRoomPrefix = this.config.get<string>('LIVEKIT_ROOM_PREFIX') ?? 'voxter';

    if (this.livekitApiKey && this.livekitApiSecret) {
      this.roomServiceClient = new RoomServiceClient(
        this.livekitRestUrl,
        this.livekitApiKey,
        this.livekitApiSecret,
      );
    } else {
      this.roomServiceClient = null;
    }
  }

  async join(userId: string, dto: JoinVoiceDto) {
    const channel = await this.prisma.channel.findUnique({ where: { id: dto.channelId } });
    if (!channel || channel.type !== ChannelType.VOICE || channel.guildId !== dto.guildId) {
      throw new BadRequestException('Invalid voice channel');
    }

    const permissions = await this.guildsService.resolveGuildPermissions(userId, dto.guildId);
    if (!hasPermission(permissions, PermissionBits.Connect)) {
      throw new ForbiddenException('Missing CONNECT permission');
    }

    const previousStates = await this.prisma.voiceState.findMany({
      where: { userId },
      select: {
        id: true,
        guildId: true,
        channelId: true,
        muted: true,
        deafened: true,
      },
    });
    const previousState = previousStates.find((state) => state.guildId === dto.guildId) ?? null;

    const state = await this.prisma.voiceState.upsert({
      where: {
        guildId_userId: {
          guildId: dto.guildId,
          userId,
        },
      },
      create: {
        guildId: dto.guildId,
        channelId: dto.channelId,
        userId,
        muted: dto.muted,
        deafened: dto.deafened,
      },
      update: {
        channelId: dto.channelId,
        muted: dto.muted,
        deafened: dto.deafened,
        joinedAt: new Date(),
      },
      include: {
        user: {
          select: { id: true, displayName: true, avatarUrl: true, presence: true },
        },
      },
    });

    const staleStates = previousStates.filter((item) => item.guildId !== dto.guildId);
    if (staleStates.length > 0) {
      await this.prisma.voiceState.deleteMany({
        where: {
          id: { in: staleStates.map((item) => item.id) },
        },
      });
      for (const staleState of staleStates) {
        this.gateway.trackVoiceState(
          staleState.guildId,
          userId,
          null,
          staleState.muted,
          staleState.deafened,
        );
        this.gateway.emitToChannel(staleState.channelId, 'voice_state_update', {
          guildId: staleState.guildId,
          channelId: staleState.channelId,
          userId,
          muted: staleState.muted,
          deafened: staleState.deafened,
          action: 'leave',
        });
      }
    }

    if (previousState && previousState.channelId !== dto.channelId) {
      this.gateway.emitToChannel(previousState.channelId, 'voice_state_update', {
        guildId: dto.guildId,
        channelId: previousState.channelId,
        userId,
        muted: previousState.muted,
        deafened: previousState.deafened,
        action: 'leave',
      });
    }

    this.gateway.emitToChannel(dto.channelId, 'voice_state_update', {
      guildId: dto.guildId,
      channelId: dto.channelId,
      userId,
      muted: state.muted,
      deafened: state.deafened,
      joinedAt: state.joinedAt,
      user: state.user,
      previousChannelId: previousState?.channelId ?? null,
      action: 'join',
    });
    this.gateway.trackVoiceState(dto.guildId, userId, dto.channelId, state.muted, state.deafened);

    return state;
  }

  async leave(userId: string, guildId: string) {
    const state = await this.prisma.voiceState.findUnique({
      where: {
        guildId_userId: {
          guildId,
          userId,
        },
      },
    });

    if (!state) {
      return { ok: true };
    }

    await this.prisma.voiceState.delete({ where: { id: state.id } });
    this.gateway.trackVoiceState(guildId, userId, null, state.muted, state.deafened);

    this.gateway.emitToChannel(state.channelId, 'voice_state_update', {
      guildId,
      channelId: state.channelId,
      userId,
      muted: state.muted,
      deafened: state.deafened,
      action: 'leave',
    });

    return { ok: true };
  }

  async updateState(userId: string, dto: UpdateVoiceStateDto) {
    const state = await this.prisma.voiceState.findUnique({
      where: {
        guildId_userId: {
          guildId: dto.guildId,
          userId,
        },
      },
    });

    if (!state) {
      throw new NotFoundException('Voice state not found');
    }

    const updated = await this.prisma.voiceState.update({
      where: { id: state.id },
      data: {
        muted: dto.muted,
        deafened: dto.deafened,
      },
    });

    this.gateway.emitToChannel(updated.channelId, 'voice_state_update', {
      guildId: dto.guildId,
      channelId: updated.channelId,
      userId,
      muted: updated.muted,
      deafened: updated.deafened,
      action: 'update',
    });
    this.gateway.trackVoiceState(dto.guildId, userId, updated.channelId, updated.muted, updated.deafened);

    return updated;
  }

  async issueSfuToken(userId: string, guildId: string, channelId: string) {
    if (!this.livekitApiKey || !this.livekitApiSecret) {
      throw new ServiceUnavailableException('LiveKit is not configured');
    }

    const channel = await this.prisma.channel.findUnique({ where: { id: channelId } });
    if (!channel || channel.type !== ChannelType.VOICE || channel.guildId !== guildId) {
      throw new BadRequestException('Invalid voice channel');
    }

    const permissions = await this.guildsService.resolveGuildPermissions(userId, guildId);
    if (!hasPermission(permissions, PermissionBits.Connect)) {
      throw new ForbiddenException('Missing CONNECT permission');
    }

    const user = await this.prisma.user.findUnique({
      where: { id: userId },
      select: { displayName: true },
    });

    const roomName = `${this.livekitRoomPrefix}:${guildId}:${channelId}`;

    if (this.roomServiceClient) {
      try {
        await this.roomServiceClient.createRoom({
          name: roomName,
          emptyTimeout: 60 * 10,
          maxParticipants: 100,
        });
      } catch {
        // Room may already exist; safe to continue.
      }
    }

    const token = new AccessToken(this.livekitApiKey, this.livekitApiSecret, {
      identity: userId,
      name: user?.displayName ?? userId,
    });

    token.addGrant({
      roomJoin: true,
      room: roomName,
      canPublish: true,
      canSubscribe: true,
      canPublishData: true,
    });

    return {
      provider: 'livekit',
      url: this.livekitWsUrl,
      roomName,
      token: await token.toJwt(),
      identity: userId,
      ttlSeconds: 900,
    };
  }

  async listParticipants(userId: string, channelId: string) {
    const channel = await this.prisma.channel.findUnique({ where: { id: channelId } });
    if (!channel) {
      throw new NotFoundException('Channel not found');
    }

    await this.guildsService.ensureMember(userId, channel.guildId);

    const staleStates = await this.prisma.voiceState.findMany({
      where: {
        channelId,
        user: { presence: 'OFFLINE' },
      },
      select: {
        id: true,
        guildId: true,
        channelId: true,
        userId: true,
        muted: true,
        deafened: true,
      },
    });
    if (staleStates.length > 0) {
      await this.prisma.voiceState.deleteMany({
        where: {
          id: { in: staleStates.map((item) => item.id) },
        },
      });
      for (const state of staleStates) {
        this.gateway.emitToChannel(state.channelId, 'voice_state_update', {
          guildId: state.guildId,
          channelId: state.channelId,
          userId: state.userId,
          muted: state.muted,
          deafened: state.deafened,
          action: 'leave',
        });
      }
    }

    return this.prisma.voiceState.findMany({
      where: {
        channelId,
        user: { presence: { not: 'OFFLINE' } },
      },
      include: {
        user: {
          select: { id: true, displayName: true, avatarUrl: true, presence: true },
        },
      },
    });
  }
}
