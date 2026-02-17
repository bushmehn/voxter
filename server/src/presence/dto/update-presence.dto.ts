import { IsEnum } from 'class-validator';
import { PresenceStatus } from '@prisma/client';

export class UpdatePresenceDto {
  @IsEnum(PresenceStatus)
  status!: PresenceStatus;
}
