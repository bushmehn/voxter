import { Module } from '@nestjs/common';
import { JwtModule } from '@nestjs/jwt';
import { GatewayService } from './gateway.service';

@Module({
  imports: [JwtModule.register({})],
  providers: [GatewayService],
  exports: [GatewayService],
})
export class GatewayModule {}
