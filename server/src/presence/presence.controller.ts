import { Body, Controller, Get, Param, Patch, UseGuards } from '@nestjs/common';
import { ApiBearerAuth, ApiTags } from '@nestjs/swagger';
import { CurrentUser, CurrentUserShape } from '../common/decorators/current-user.decorator';
import { JwtAuthGuard } from '../common/guards/jwt-auth.guard';
import { UpdatePresenceDto } from './dto/update-presence.dto';
import { PresenceService } from './presence.service';

@ApiTags('presence')
@ApiBearerAuth()
@UseGuards(JwtAuthGuard)
@Controller('presence')
export class PresenceController {
  constructor(private readonly presenceService: PresenceService) {}

  @Patch('me')
  async update(@CurrentUser() user: CurrentUserShape, @Body() dto: UpdatePresenceDto) {
    return this.presenceService.updateStatus(user.userId, dto.status);
  }

  @Get('guild/:guildId')
  async listGuild(@CurrentUser() user: CurrentUserShape, @Param('guildId') guildId: string) {
    return this.presenceService.listGuildPresence(user.userId, guildId);
  }
}
