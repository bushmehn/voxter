import { Module } from '@nestjs/common';
import { GatewayModule } from '../gateway/gateway.module';
import { DmController } from './dm.controller';
import { DmService } from './dm.service';

@Module({
  imports: [GatewayModule],
  controllers: [DmController],
  providers: [DmService],
  exports: [DmService],
})
export class DmModule {}
