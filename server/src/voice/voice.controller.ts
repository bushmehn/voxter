import {
  Body,
  Controller,
  Get,
  Param,
  Patch,
  Post,
  UseGuards,
} from '@nestjs/common';
import { ApiBearerAuth, ApiTags } from '@nestjs/swagger';
import { CurrentUser, CurrentUserShape } from '../common/decorators/current-user.decorator';
import { JwtAuthGuard } from '../common/guards/jwt-auth.guard';
import { JoinVoiceDto } from './dto/join-voice.dto';
import { LeaveVoiceDto } from './dto/leave-voice.dto';
import { SfuTokenDto } from './dto/sfu-token.dto';
import { UpdateVoiceStateDto } from './dto/update-voice.dto';
import { VoiceService } from './voice.service';

@ApiTags('voice')
@ApiBearerAuth()
@UseGuards(JwtAuthGuard)
@Controller('voice')
export class VoiceController {
  constructor(private readonly voiceService: VoiceService) {}

  @Post('join')
  async join(@CurrentUser() user: CurrentUserShape, @Body() dto: JoinVoiceDto) {
    return this.voiceService.join(user.userId, dto);
  }

  @Post('sfu-token')
  async sfuToken(@CurrentUser() user: CurrentUserShape, @Body() dto: SfuTokenDto) {
    return this.voiceService.issueSfuToken(user.userId, dto.guildId, dto.channelId);
  }

  @Post('leave')
  async leave(@CurrentUser() user: CurrentUserShape, @Body() dto: LeaveVoiceDto) {
    return this.voiceService.leave(user.userId, dto.guildId);
  }

  @Patch('state')
  async update(@CurrentUser() user: CurrentUserShape, @Body() dto: UpdateVoiceStateDto) {
    return this.voiceService.updateState(user.userId, dto);
  }

  @Get('channel/:channelId')
  async listParticipants(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
  ) {
    return this.voiceService.listParticipants(user.userId, channelId);
  }
}
