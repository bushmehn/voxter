import { IsString, MaxLength } from 'class-validator';

export class UpdateMessageDto {
  @IsString()
  @MaxLength(4000)
  content!: string;
}
