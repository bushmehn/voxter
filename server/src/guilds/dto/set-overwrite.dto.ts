import { IsIn, IsString, Matches } from 'class-validator';

export class SetOverwriteDto {
  @IsString()
  @Matches(/^\d+$/)
  allow!: string;

  @IsString()
  @Matches(/^\d+$/)
  deny!: string;

  @IsString()
  subjectId!: string;

  @IsString()
  @IsIn(['role', 'user'])
  subjectType!: 'role' | 'user';
}
