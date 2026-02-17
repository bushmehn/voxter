import { Controller, Get, INestApplication } from '@nestjs/common';
import { Test } from '@nestjs/testing';
import request from 'supertest';

@Controller('health')
class HealthController {
  @Get()
  health() {
    return { ok: true };
  }
}

describe('App e2e happy path', () => {
  let app: INestApplication;

  beforeAll(async () => {
    const moduleRef = await Test.createTestingModule({
      controllers: [HealthController],
    }).compile();

    app = moduleRef.createNestApplication();
    await app.init();
  });

  afterAll(async () => {
    await app.close();
  });

  it('/health returns service status', async () => {
    await request(app.getHttpServer())
      .get('/health')
      .expect(200)
      .expect({ ok: true });
  });
});
