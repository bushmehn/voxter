import { IsOptional, IsString, MaxLength, Matches, IsInt } from 'class-validator';

export class UpdateRoleDto {
  @IsOptional()
  @IsString()
  @MaxLength(32)
  name?: string;

  @IsOptional()
  @Matches(/^#[0-9A-Fa-f]{6}$/)
  color?: string;

  @IsOptional()
  @IsString()
  @Matches(/^\d+$/)
  permissions?: string;

  @IsOptional()
  @IsInt()
  position?: number;
}
