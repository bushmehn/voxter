import { Injectable, Logger, OnModuleDestroy } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import Redis from 'ioredis';

@Injectable()
export class RedisService implements OnModuleDestroy {
  private readonly logger = new Logger(RedisService.name);
  private readonly client: Redis;

  constructor(config: ConfigService) {
    const url = config.getOrThrow<string>('REDIS_URL');
    this.client = new Redis(url, {
      lazyConnect: false,
      maxRetriesPerRequest: 3,
    });

    this.client.on('error', (error) => {
      this.logger.error(`Redis error: ${error.message}`);
    });
  }

  get raw() {
    return this.client;
  }

  async setPresence(userId: string, status: string) {
    await this.client.set(`presence:${userId}`, status, 'EX', 300);
  }

  async getPresence(userId: string): Promise<string | null> {
    return this.client.get(`presence:${userId}`);
  }

  async onModuleDestroy() {
    await this.client.quit();
  }
}
