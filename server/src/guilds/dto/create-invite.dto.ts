import { IsOptional, IsInt, Min, IsDateString } from 'class-validator';

export class CreateInviteDto {
  @IsOptional()
  @IsInt()
  @Min(1)
  maxUses?: number;

  @IsOptional()
  @IsDateString()
  expiresAt?: string;
}
