import { IsString, MinLength, MaxLength } from 'class-validator';

export class CreateGuildDto {
  @IsString()
  @MinLength(2)
  @MaxLength(64)
  name!: string;
}
