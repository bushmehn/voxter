import { IsString } from 'class-validator';

export class LeaveVoiceDto {
  @IsString()
  guildId!: string;
}
