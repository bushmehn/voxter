import { ArrayMinSize, IsArray, IsOptional, IsString, MaxLength } from 'class-validator';

export class CreateThreadDto {
  @IsArray()
  @ArrayMinSize(1)
  @IsString({ each: true })
  participantIds!: string[];

  @IsOptional()
  @IsString()
  @MaxLength(64)
  name?: string;
}
