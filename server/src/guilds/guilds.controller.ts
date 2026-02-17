import {
  Body,
  Controller,
  Get,
  Param,
  Post,
  Patch,
  UseGuards,
} from '@nestjs/common';
import { ApiBearerAuth, ApiTags } from '@nestjs/swagger';
import { CurrentUser, CurrentUserShape } from '../common/decorators/current-user.decorator';
import { JwtAuthGuard } from '../common/guards/jwt-auth.guard';
import { CreateChannelDto } from './dto/create-channel.dto';
import { CreateGuildDto } from './dto/create-guild.dto';
import { CreateInviteDto } from './dto/create-invite.dto';
import { CreateRoleDto } from './dto/create-role.dto';
import { SetOverwriteDto } from './dto/set-overwrite.dto';
import { UpdateRoleDto } from './dto/update-role.dto';
import { GuildsService } from './guilds.service';

@ApiTags('guilds')
@ApiBearerAuth()
@UseGuards(JwtAuthGuard)
@Controller('guilds')
export class GuildsController {
  constructor(private readonly guildsService: GuildsService) {}

  @Get('me')
  async listMyGuilds(@CurrentUser() user: CurrentUserShape) {
    return this.guildsService.listMyGuilds(user.userId);
  }

  @Post()
  async createGuild(@CurrentUser() user: CurrentUserShape, @Body() dto: CreateGuildDto) {
    return this.guildsService.createGuild(user.userId, dto);
  }

  @Get(':guildId/channels')
  async listChannels(@CurrentUser() user: CurrentUserShape, @Param('guildId') guildId: string) {
    return this.guildsService.listChannels(user.userId, guildId);
  }

  @Post(':guildId/channels')
  async createChannel(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Body() dto: CreateChannelDto,
  ) {
    return this.guildsService.createChannel(user.userId, guildId, dto);
  }

  @Post(':guildId/invites')
  async createInvite(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Body() dto: CreateInviteDto,
  ) {
    return this.guildsService.createInvite(user.userId, guildId, dto);
  }

  @Post('join/:code')
  async joinByInvite(@CurrentUser() user: CurrentUserShape, @Param('code') code: string) {
    return this.guildsService.joinByInvite(user.userId, code);
  }

  @Get(':guildId/roles')
  async listRoles(@CurrentUser() user: CurrentUserShape, @Param('guildId') guildId: string) {
    return this.guildsService.listRoles(user.userId, guildId);
  }

  @Post(':guildId/roles')
  async createRole(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Body() dto: CreateRoleDto,
  ) {
    return this.guildsService.createRole(user.userId, guildId, dto);
  }

  @Patch(':guildId/roles/:roleId')
  async updateRole(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Param('roleId') roleId: string,
    @Body() dto: UpdateRoleDto,
  ) {
    return this.guildsService.updateRole(user.userId, guildId, roleId, dto);
  }

  @Post(':guildId/members/:memberId/roles/:roleId')
  async assignRole(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Param('memberId') memberId: string,
    @Param('roleId') roleId: string,
  ) {
    return this.guildsService.assignRole(user.userId, guildId, memberId, roleId);
  }

  @Post(':guildId/channels/:channelId/overwrites')
  async setOverwrite(
    @CurrentUser() user: CurrentUserShape,
    @Param('guildId') guildId: string,
    @Param('channelId') channelId: string,
    @Body() dto: SetOverwriteDto,
  ) {
    return this.guildsService.setOverwrite(user.userId, guildId, channelId, dto);
  }
}
