import { Test } from '@nestjs/testing';
import { JwtModule } from '@nestjs/jwt';
import { ConfigService } from '@nestjs/config';
import * as argon2 from 'argon2';
import { AuthService } from '../../src/auth/auth.service';
import { PrismaService } from '../../src/prisma/prisma.service';

const createMockPrisma = () => {
  const users: any[] = [];
  const refreshTokens: any[] = [];

  return {
    user: {
      findUnique: jest.fn(async ({ where }: any) => {
        if (where.email) {
          return users.find((u) => u.email === where.email) ?? null;
        }
        return users.find((u) => u.id === where.id) ?? null;
      }),
      create: jest.fn(async ({ data }: any) => {
        const user = {
          id: `user-${users.length + 1}`,
          email: data.email,
          passwordHash: data.passwordHash,
          displayName: data.displayName,
          avatarUrl: null,
          presence: 'OFFLINE',
        };
        users.push(user);
        return user;
      }),
    },
    refreshToken: {
      create: jest.fn(async ({ data }: any) => {
        refreshTokens.push({ ...data, revokedAt: null });
        return data;
      }),
      findUnique: jest.fn(async ({ where }: any) => {
        const found = refreshTokens.find((token) => token.id === where.id);
        if (!found) {
          return null;
        }
        const user = users.find((item) => item.id === found.userId);
        return { ...found, user };
      }),
      update: jest.fn(async ({ where, data }: any) => {
        const index = refreshTokens.findIndex((token) => token.id === where.id);
        refreshTokens[index] = { ...refreshTokens[index], ...data };
        return refreshTokens[index];
      }),
      updateMany: jest.fn(async ({ where, data }: any) => {
        for (let i = 0; i < refreshTokens.length; i += 1) {
          if (refreshTokens[i].userId === where.userId) {
            refreshTokens[i] = { ...refreshTokens[i], ...data };
          }
        }
        return { count: refreshTokens.length };
      }),
    },
  } as unknown as PrismaService;
};

describe('AuthService integration', () => {
  it('registers, logins and refreshes tokens', async () => {
    const mockPrisma = createMockPrisma();

    const module = await Test.createTestingModule({
      imports: [JwtModule.register({})],
      providers: [
        AuthService,
        {
          provide: PrismaService,
          useValue: mockPrisma,
        },
        {
          provide: ConfigService,
          useValue: {
            get: (_key: string, fallback?: string) => fallback,
            getOrThrow: (key: string) => {
              if (key === 'JWT_ACCESS_SECRET') return 'access-secret';
              if (key === 'JWT_ACCESS_TTL') return '15m';
              if (key === 'JWT_REFRESH_TTL_DAYS') return '7';
              if (key === 'JWT_REFRESH_SECRET') return 'refresh-secret';
              return 'value';
            },
          },
        },
      ],
    }).compile();

    const service = module.get(AuthService);

    const registerResult = await service.register({
      email: 'user@voxter.dev',
      password: 'Passw0rd!!',
      displayName: 'User',
    });

    expect(registerResult.user.email).toBe('user@voxter.dev');
    expect(registerResult.accessToken).toBeTruthy();
    expect(registerResult.refreshToken).toContain('.');

    const loginResult = await service.login({
      email: 'user@voxter.dev',
      password: 'Passw0rd!!',
    });

    expect(loginResult.accessToken).toBeTruthy();

    const parsed = loginResult.refreshToken.split('.');
    const tokenRecord = await (mockPrisma as any).refreshToken.findUnique({ where: { id: parsed[0] } });
    expect(await argon2.verify(tokenRecord.tokenHash, parsed[1])).toBe(true);

    const refreshed = await service.refresh(loginResult.refreshToken);
    expect(refreshed.accessToken).toBeTruthy();
    expect(refreshed.refreshToken).not.toBe(loginResult.refreshToken);
  });
});
