import { IsBoolean, IsString } from 'class-validator';

export class UpdateVoiceStateDto {
  @IsString()
  guildId!: string;

  @IsBoolean()
  muted!: boolean;

  @IsBoolean()
  deafened!: boolean;
}
