import { IsBoolean, IsString } from 'class-validator';

export class JoinVoiceDto {
  @IsString()
  guildId!: string;

  @IsString()
  channelId!: string;

  @IsBoolean()
  muted!: boolean;

  @IsBoolean()
  deafened!: boolean;
}
