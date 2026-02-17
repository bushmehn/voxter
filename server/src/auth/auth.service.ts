import {
  BadRequestException,
  Injectable,
  UnauthorizedException,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { JwtService } from '@nestjs/jwt';
import { User } from '@prisma/client';
import * as argon2 from 'argon2';
import { randomBytes } from 'crypto';
import type { StringValue } from 'ms';
import { JwtPayload } from '../common/interfaces/jwt-payload.interface';
import { sanitizePlainText } from '../common/sanitize';
import { PrismaService } from '../prisma/prisma.service';
import { LoginDto } from './dto/login.dto';
import { RegisterDto } from './dto/register.dto';

interface TokenPair {
  accessToken: string;
  refreshToken: string;
}

@Injectable()
export class AuthService {
  constructor(
    private readonly prisma: PrismaService,
    private readonly jwtService: JwtService,
    private readonly config: ConfigService,
  ) {}

  async register(dto: RegisterDto) {
    const normalizedEmail = dto.email.toLowerCase();
    const existing = await this.prisma.user.findUnique({ where: { email: normalizedEmail } });
    if (existing) {
      throw new BadRequestException('Email already registered');
    }

    const user = await this.prisma.user.create({
      data: {
        email: normalizedEmail,
        passwordHash: await argon2.hash(dto.password),
        displayName: sanitizePlainText(dto.displayName),
      },
    });

    const tokens = await this.issueTokenPair(user);
    return { user: this.toUserDto(user), ...tokens };
  }

  async login(dto: LoginDto) {
    const user = await this.prisma.user.findUnique({ where: { email: dto.email.toLowerCase() } });
    if (!user) {
      throw new UnauthorizedException('Invalid credentials');
    }

    const valid = await argon2.verify(user.passwordHash, dto.password);
    if (!valid) {
      throw new UnauthorizedException('Invalid credentials');
    }

    const tokens = await this.issueTokenPair(user);
    return { user: this.toUserDto(user), ...tokens };
  }

  async refresh(rawRefreshToken: string) {
    const parsed = this.parseRefreshToken(rawRefreshToken);
    if (!parsed) {
      throw new UnauthorizedException('Invalid refresh token');
    }

    const tokenRecord = await this.prisma.refreshToken.findUnique({
      where: { id: parsed.tokenId },
      include: { user: true },
    });

    if (!tokenRecord || tokenRecord.revokedAt || tokenRecord.expiresAt < new Date()) {
      throw new UnauthorizedException('Refresh token expired');
    }

    const tokenValid = await argon2.verify(tokenRecord.tokenHash, parsed.secret);
    if (!tokenValid) {
      throw new UnauthorizedException('Invalid refresh token');
    }

    await this.prisma.refreshToken.update({
      where: { id: tokenRecord.id },
      data: { revokedAt: new Date() },
    });

    const tokens = await this.issueTokenPair(tokenRecord.user, tokenRecord.id);
    return { user: this.toUserDto(tokenRecord.user), ...tokens };
  }

  async revokeRefreshToken(rawRefreshToken: string): Promise<boolean> {
    const parsed = this.parseRefreshToken(rawRefreshToken);
    if (!parsed) {
      return false;
    }

    const token = await this.prisma.refreshToken.findUnique({ where: { id: parsed.tokenId } });
    if (!token) {
      return false;
    }

    await this.prisma.refreshToken.update({
      where: { id: token.id },
      data: { revokedAt: new Date() },
    });

    return true;
  }

  async revokeAll(userId: string, accessToken: string) {
    const payload = this.decodeAccessToken(accessToken);
    if (!payload || payload.sub !== userId) {
      throw new UnauthorizedException('Invalid access token');
    }

    await this.prisma.refreshToken.updateMany({
      where: { userId, revokedAt: null },
      data: { revokedAt: new Date() },
    });
  }

  async getProfile(userId: string) {
    const user = await this.prisma.user.findUnique({ where: { id: userId } });
    if (!user) {
      throw new UnauthorizedException('User not found');
    }

    return this.toUserDto(user);
  }

  private decodeAccessToken(token: string): JwtPayload | null {
    try {
      return this.jwtService.verify<JwtPayload>(token, {
        secret: this.config.getOrThrow<string>('JWT_ACCESS_SECRET'),
      });
    } catch {
      return null;
    }
  }

  private async issueTokenPair(user: User, rotatedFromId?: string): Promise<TokenPair> {
    const payload: JwtPayload = {
      sub: user.id,
      email: user.email,
      tokenVersion: randomBytes(8).toString('hex'),
    };

    const accessTtlRaw = this.config.get<string>('JWT_ACCESS_TTL', '15m');
    const accessExpiresIn: number | StringValue = /^\d+$/.test(accessTtlRaw)
      ? Number(accessTtlRaw)
      : (accessTtlRaw as StringValue);

    const accessToken = await this.jwtService.signAsync(payload, {
      secret: this.config.getOrThrow<string>('JWT_ACCESS_SECRET'),
      expiresIn: accessExpiresIn,
    });

    const refreshSecret = randomBytes(48).toString('base64url');
    const refreshTokenId = randomBytes(16).toString('hex');
    const refreshToken = `${refreshTokenId}.${refreshSecret}`;

    const refreshTtlDays = Number(this.config.get<string>('JWT_REFRESH_TTL_DAYS', '7'));
    const expiresAt = new Date(Date.now() + refreshTtlDays * 24 * 60 * 60 * 1000);

    await this.prisma.refreshToken.create({
      data: {
        id: refreshTokenId,
        userId: user.id,
        tokenHash: await argon2.hash(refreshSecret),
        expiresAt,
        rotatedFromId,
      },
    });

    return { accessToken, refreshToken };
  }

  private parseRefreshToken(value: string): { tokenId: string; secret: string } | null {
    const parts = value.split('.');
    if (parts.length !== 2) {
      return null;
    }
    if (parts[0].length < 12 || parts[1].length < 32) {
      return null;
    }
    return { tokenId: parts[0], secret: parts[1] };
  }

  private toUserDto(user: User) {
    return {
      id: user.id,
      email: user.email,
      displayName: user.displayName,
      avatarUrl: user.avatarUrl,
      presence: user.presence,
    };
  }
}
