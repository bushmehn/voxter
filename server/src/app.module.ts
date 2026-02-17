import { Module } from '@nestjs/common';
import { ConfigModule } from '@nestjs/config';
import { ThrottlerGuard, ThrottlerModule } from '@nestjs/throttler';
import { APP_GUARD } from '@nestjs/core';
import { validateEnv } from './config/env.validation';
import { PrismaModule } from './prisma/prisma.module';
import { RedisModule } from './redis/redis.module';
import { AuthModule } from './auth/auth.module';
import { UsersModule } from './users/users.module';
import { GuildsModule } from './guilds/guilds.module';
import { MessagesModule } from './messages/messages.module';
import { DmModule } from './dm/dm.module';
import { PresenceModule } from './presence/presence.module';
import { VoiceModule } from './voice/voice.module';
import { GatewayModule } from './gateway/gateway.module';

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
      validate: validateEnv,
    }),
    ThrottlerModule.forRoot([
      {
        ttl: 60_000,
        limit: 240,
      },
    ]),
    PrismaModule,
    RedisModule,
    AuthModule,
    UsersModule,
    GuildsModule,
    MessagesModule,
    DmModule,
    PresenceModule,
    VoiceModule,
    GatewayModule,
  ],
  providers: [
    {
      provide: APP_GUARD,
      useClass: ThrottlerGuard,
    },
  ],
})
export class AppModule {}
