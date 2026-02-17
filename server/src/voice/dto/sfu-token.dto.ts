import { IsString } from 'class-validator';

export class SfuTokenDto {
  @IsString()
  guildId!: string;

  @IsString()
  channelId!: string;
}
