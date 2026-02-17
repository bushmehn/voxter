import { Test } from '@nestjs/testing';
import { ConfigService } from '@nestjs/config';
import { MessagesService } from '../../src/messages/messages.service';
import { PermissionBits } from '../../src/common/constants/permissions';
import { PrismaService } from '../../src/prisma/prisma.service';
import { GuildsService } from '../../src/guilds/guilds.service';
import { GatewayService } from '../../src/gateway/gateway.service';

describe('MessagesService integration', () => {
  it('creates a channel message and emits gateway event', async () => {
    const gateway = {
      emitToChannel: jest.fn(),
    } as unknown as GatewayService;

    const prisma = {
      channel: {
        findUnique: jest.fn(async () => ({ id: 'channel-1', guildId: 'guild-1', overwrites: [] })),
      },
      guildMemberRole: {
        findMany: jest.fn(async () => []),
      },
      message: {
        create: jest.fn(async ({ data }: any) => ({
          id: 'message-1',
          channelId: data.channelId,
          dmThreadId: null,
          authorId: data.authorId,
          content: data.content,
          editedAt: null,
          deletedAt: null,
          createdAt: new Date(),
          updatedAt: new Date(),
          author: { id: data.authorId, displayName: 'User' },
          attachments: [],
          reactions: [],
        })),
      },
    } as unknown as PrismaService;

    const guilds = {
      resolveGuildPermissions: jest.fn(async () => PermissionBits.SendMessages | PermissionBits.ViewChannel),
    } as unknown as GuildsService;

    const module = await Test.createTestingModule({
      providers: [
        MessagesService,
        {
          provide: PrismaService,
          useValue: prisma,
        },
        {
          provide: GuildsService,
          useValue: guilds,
        },
        {
          provide: GatewayService,
          useValue: gateway,
        },
        {
          provide: ConfigService,
          useValue: {
            get: (_key: string, fallback?: string) => fallback,
          },
        },
      ],
    }).compile();

    const service = module.get(MessagesService);
    const message = await service.createChannelMessage('user-1', 'channel-1', {
      content: '<b>Hello</b>',
    });

    expect(message.id).toBe('message-1');
    expect((gateway.emitToChannel as jest.Mock).mock.calls.length).toBe(1);
  });
});
