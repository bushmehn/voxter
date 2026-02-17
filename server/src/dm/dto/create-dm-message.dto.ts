import { IsString, MaxLength } from 'class-validator';

export class CreateDmMessageDto {
  @IsString()
  @MaxLength(4000)
  content!: string;
}
