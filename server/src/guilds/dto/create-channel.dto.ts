import { IsEnum, IsString, MinLength, MaxLength, IsOptional, IsInt, Min } from 'class-validator';
import { ChannelType } from '@prisma/client';

export class CreateChannelDto {
  @IsString()
  @MinLength(1)
  @MaxLength(48)
  name!: string;

  @IsEnum(ChannelType)
  type!: ChannelType;

  @IsOptional()
  @IsInt()
  @Min(0)
  position?: number;
}
