import { Injectable, NotFoundException } from '@nestjs/common';
import { sanitizePlainText } from '../common/sanitize';
import { PrismaService } from '../prisma/prisma.service';
import { UpdateProfileDto } from './dto/update-profile.dto';

@Injectable()
export class UsersService {
  constructor(private readonly prisma: PrismaService) {}

  async updateProfile(userId: string, dto: UpdateProfileDto) {
    const updated = await this.prisma.user.update({
      where: { id: userId },
      data: {
        displayName: dto.displayName ? sanitizePlainText(dto.displayName) : undefined,
      },
    });

    if (!updated) {
      throw new NotFoundException('User not found');
    }

    return {
      id: updated.id,
      email: updated.email,
      displayName: updated.displayName,
      avatarUrl: updated.avatarUrl,
      presence: updated.presence,
    };
  }
}
