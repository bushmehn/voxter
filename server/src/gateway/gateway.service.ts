import { Injectable, Logger } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { JwtService } from '@nestjs/jwt';
import {
  ConnectedSocket,
  MessageBody,
  OnGatewayConnection,
  OnGatewayDisconnect,
  SubscribeMessage,
  WebSocketGateway,
} from '@nestjs/websockets';
import { IncomingMessage } from 'http';
import { URL } from 'url';
import { WebSocket } from 'ws';
import { JwtPayload } from '../common/interfaces/jwt-payload.interface';
import { PrismaService } from '../prisma/prisma.service';
import { RedisService } from '../redis/redis.service';

interface GatewayEnvelope {
  v: '1.0';
  t: string;
  d: unknown;
}

interface ClientSession {
  userId: string;
  email: string;
  channelSubscriptions: Set<string>;
  dmSubscriptions: Set<string>;
  guildSubscriptions: Set<string>;
}

interface VoiceSessionState {
  channelId: string;
  muted: boolean;
  deafened: boolean;
}

@WebSocketGateway({ path: '/gateway' })
@Injectable()
export class GatewayService implements OnGatewayConnection, OnGatewayDisconnect {
  private readonly logger = new Logger(GatewayService.name);
  private readonly sessions = new WeakMap<WebSocket, ClientSession>();
  private readonly socketsByUserId = new Map<string, Set<WebSocket>>();
  private readonly voiceSessionsByGuildUser = new Map<string, VoiceSessionState>();

  constructor(
    private readonly jwtService: JwtService,
    private readonly config: ConfigService,
    private readonly prisma: PrismaService,
    private readonly redis: RedisService,
  ) {}

  async handleConnection(client: WebSocket, request: IncomingMessage) {
    const token = this.extractToken(request);
    if (!token) {
      client.close(4001, 'Unauthorized');
      return;
    }

    const payload = this.verifyToken(token);
    if (!payload) {
      client.close(4001, 'Unauthorized');
      return;
    }

    const user = await this.prisma.user.findUnique({
      where: { id: payload.sub },
      select: { id: true, email: true },
    });

    if (!user) {
      client.close(4001, 'Unauthorized');
      return;
    }

    const session: ClientSession = {
      userId: user.id,
      email: user.email,
      channelSubscriptions: new Set<string>(),
      dmSubscriptions: new Set<string>(),
      guildSubscriptions: new Set<string>(),
    };

    this.sessions.set(client, session);
    const sockets = this.socketsByUserId.get(session.userId) ?? new Set<WebSocket>();
    sockets.add(client);
    this.socketsByUserId.set(session.userId, sockets);

    await this.redis.setPresence(session.userId, 'ONLINE');
    await this.prisma.user.update({
      where: { id: session.userId },
      data: { presence: 'ONLINE' },
    });

    this.broadcast('presence_update', {
      userId: session.userId,
      status: 'ONLINE',
    });

    this.send(client, 'hello', {
      userId: session.userId,
      serverTime: new Date().toISOString(),
    });
  }

  async handleDisconnect(client: WebSocket) {
    const session = this.sessions.get(client);
    if (!session) {
      return;
    }

    const sockets = this.socketsByUserId.get(session.userId);
    if (sockets) {
      sockets.delete(client);
      if (sockets.size === 0) {
        this.socketsByUserId.delete(session.userId);

        const activeVoiceStates = await this.prisma.voiceState.findMany({
          where: { userId: session.userId },
          select: {
            guildId: true,
            channelId: true,
            muted: true,
            deafened: true,
          },
        });
        if (activeVoiceStates.length > 0) {
          await this.prisma.voiceState.deleteMany({ where: { userId: session.userId } });
          for (const state of activeVoiceStates) {
            this.trackVoiceState(state.guildId, session.userId, null, state.muted, state.deafened);
            this.emitToChannel(state.channelId, 'voice_state_update', {
              guildId: state.guildId,
              channelId: state.channelId,
              userId: session.userId,
              muted: state.muted,
              deafened: state.deafened,
              action: 'leave',
            });
          }
        }

        await this.redis.setPresence(session.userId, 'OFFLINE');
        await this.prisma.user.update({
          where: { id: session.userId },
          data: { presence: 'OFFLINE' },
        });

        this.broadcast('presence_update', {
          userId: session.userId,
          status: 'OFFLINE',
        });
      }
    }
  }

  @SubscribeMessage('ping')
  handlePing(@ConnectedSocket() client: WebSocket) {
    this.send(client, 'pong', { now: Date.now() });
  }

  @SubscribeMessage('subscribe_channels')
  async handleSubscribeChannels(
    @ConnectedSocket() client: WebSocket,
    @MessageBody() body: { channelIds: string[] },
  ) {
    const session = this.sessions.get(client);
    if (!session) {
      return;
    }

    const requestedChannelIds = body?.channelIds ?? [];
    const memberships = await this.prisma.channel.findMany({
      where: {
        id: { in: requestedChannelIds },
        guild: { members: { some: { userId: session.userId } } },
      },
      select: { id: true, guildId: true },
    });

    for (const item of memberships) {
      session.channelSubscriptions.add(item.id);
      session.guildSubscriptions.add(item.guildId);
    }

    this.send(client, 'subscribed_channels', {
      channelIds: memberships.map((item) => item.id),
    });
  }

  @SubscribeMessage('subscribe_dm_threads')
  async handleSubscribeDm(
    @ConnectedSocket() client: WebSocket,
    @MessageBody() body: { threadIds: string[] },
  ) {
    const session = this.sessions.get(client);
    if (!session) {
      return;
    }

    const requestedThreadIds = body?.threadIds ?? [];
    const valid = await this.prisma.dMThreadMember.findMany({
      where: {
        threadId: { in: requestedThreadIds },
        userId: session.userId,
      },
      select: { threadId: true },
    });

    for (const item of valid) {
      session.dmSubscriptions.add(item.threadId);
    }

    this.send(client, 'subscribed_dm_threads', {
      threadIds: valid.map((item) => item.threadId),
    });
  }

  @SubscribeMessage('typing')
  handleTyping(
    @ConnectedSocket() client: WebSocket,
    @MessageBody() body: { channelId: string },
  ) {
    const session = this.sessions.get(client);
    if (!session || !session.channelSubscriptions.has(body.channelId)) {
      return;
    }

    this.emitToChannel(body.channelId, 'typing', {
      channelId: body.channelId,
      userId: session.userId,
    });
  }

  @SubscribeMessage('presence_set')
  async handlePresenceSet(
    @ConnectedSocket() client: WebSocket,
    @MessageBody() body: { status: 'ONLINE' | 'IDLE' | 'DND' | 'OFFLINE' },
  ) {
    const session = this.sessions.get(client);
    if (!session) {
      return;
    }

    await this.redis.setPresence(session.userId, body.status);
    await this.prisma.user.update({
      where: { id: session.userId },
      data: { presence: body.status },
    });

    this.broadcast('presence_update', {
      userId: session.userId,
      status: body.status,
    });
  }

  @SubscribeMessage('voice_signal')
  handleVoiceSignal(
    @ConnectedSocket() client: WebSocket,
    @MessageBody()
    body: {
      channelId: string;
      targetUserId: string;
      signal: Record<string, unknown>;
    },
  ) {
    const session = this.sessions.get(client);
    if (!session || !session.channelSubscriptions.has(body.channelId)) {
      this.logger.warn(
        `voice_signal rejected sender=${session?.userId ?? 'unknown'} channel=${body.channelId}`,
      );
      return;
    }

    this.emitToUser(body.targetUserId, 'voice_signal', {
      channelId: body.channelId,
      sourceUserId: session.userId,
      signal: body.signal,
    });
  }

  @SubscribeMessage('voice_audio')
  async handleVoiceAudio(
    @ConnectedSocket() client: WebSocket,
    @MessageBody()
    body: {
      guildId: string;
      channelId: string;
      pcm: string;
      sampleRate: number;
      channels: number;
      sampleFormat: string;
    },
  ) {
    const session = this.sessions.get(client);
    if (!session || !body?.guildId || !body?.channelId || !body?.pcm) {
      return;
    }

    if (!session.channelSubscriptions.has(body.channelId)) {
      return;
    }

    const voiceState = this.voiceSessionsByGuildUser.get(this.makeVoiceSessionKey(body.guildId, session.userId));
    if (!voiceState || voiceState.channelId !== body.channelId || voiceState.muted || voiceState.deafened) {
      return;
    }

    this.emitToChannel(body.channelId, 'voice_audio', {
      guildId: body.guildId,
      channelId: body.channelId,
      userId: session.userId,
      pcm: body.pcm,
      sampleRate: body.sampleRate ?? 16000,
      channels: body.channels ?? 1,
      sampleFormat: body.sampleFormat ?? 's16le',
      at: new Date().toISOString(),
    });
  }

  @SubscribeMessage('voice_speaking')
  async handleVoiceSpeaking(
    @ConnectedSocket() client: WebSocket,
    @MessageBody()
    body: {
      guildId: string;
      channelId: string;
      speaking: boolean;
    },
  ) {
    const session = this.sessions.get(client);
    if (!session || !body?.guildId || !body?.channelId) {
      return;
    }

    if (!session.channelSubscriptions.has(body.channelId)) {
      return;
    }

    const voiceState = await this.prisma.voiceState.findUnique({
      where: {
        guildId_userId: {
          guildId: body.guildId,
          userId: session.userId,
        },
      },
      select: {
        channelId: true,
      },
    });

    if (!voiceState || voiceState.channelId !== body.channelId) {
      return;
    }

    this.emitToChannel(body.channelId, 'voice_speaking_update', {
      guildId: body.guildId,
      channelId: body.channelId,
      userId: session.userId,
      speaking: !!body.speaking,
      at: new Date().toISOString(),
    });
  }

  emitToChannel(channelId: string, event: string, data: unknown) {
    this.forEachSession((socket, session) => {
      if (session.channelSubscriptions.has(channelId)) {
        this.send(socket, event, data);
      }
    });
  }

  emitToGuild(guildId: string, event: string, data: unknown) {
    this.forEachSession((socket, session) => {
      if (session.guildSubscriptions.has(guildId)) {
        this.send(socket, event, data);
      }
    });
  }

  emitToDmThread(threadId: string, event: string, data: unknown) {
    this.forEachSession((socket, session) => {
      if (session.dmSubscriptions.has(threadId)) {
        this.send(socket, event, data);
      }
    });
  }

  emitToUser(userId: string, event: string, data: unknown): number {
    const sockets = this.socketsByUserId.get(userId);
    if (!sockets) {
      return 0;
    }

    let delivered = 0;
    for (const socket of sockets) {
      this.send(socket, event, data);
      delivered += 1;
    }
    return delivered;
  }

  broadcast(event: string, data: unknown) {
    this.forEachSession((socket) => {
      this.send(socket, event, data);
    });
  }

  private forEachSession(visitor: (socket: WebSocket, session: ClientSession) => void) {
    for (const sockets of this.socketsByUserId.values()) {
      for (const socket of sockets) {
        const session = this.sessions.get(socket);
        if (session) {
          visitor(socket, session);
        }
      }
    }
  }

  private send(client: WebSocket, event: string, data: unknown) {
    if (client.readyState !== WebSocket.OPEN) {
      return;
    }

    const envelope: GatewayEnvelope = {
      v: '1.0',
      t: event,
      d: data,
    };

    client.send(JSON.stringify(envelope));
  }

  private extractToken(request: IncomingMessage): string | null {
    const authHeader = request.headers.authorization;
    if (authHeader?.startsWith('Bearer ')) {
      return authHeader.slice('Bearer '.length);
    }

    const host = request.headers.host ?? 'localhost';
    const requestUrl = new URL(request.url ?? '/', `http://${host}`);
    const queryToken = requestUrl.searchParams.get('token');
    if (queryToken) {
      return queryToken;
    }

    return null;
  }

  private verifyToken(token: string): JwtPayload | null {
    try {
      return this.jwtService.verify<JwtPayload>(token, {
        secret: this.config.getOrThrow<string>('JWT_ACCESS_SECRET'),
      });
    } catch (error) {
      this.logger.warn(`WS token verification failed: ${(error as Error).message}`);
      return null;
    }
  }

  trackVoiceState(
    guildId: string,
    userId: string,
    channelId: string | null,
    muted: boolean,
    deafened: boolean,
  ) {
    const key = this.makeVoiceSessionKey(guildId, userId);
    if (!channelId) {
      this.voiceSessionsByGuildUser.delete(key);
      return;
    }

    this.voiceSessionsByGuildUser.set(key, {
      channelId,
      muted,
      deafened,
    });
  }

  private makeVoiceSessionKey(guildId: string, userId: string) {
    return `${guildId}:${userId}`;
  }
}
