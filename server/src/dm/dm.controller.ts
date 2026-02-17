import { Body, Controller, Get, Param, Post, Query, UseGuards } from '@nestjs/common';
import { ApiBearerAuth, ApiTags } from '@nestjs/swagger';
import { CurrentUser, CurrentUserShape } from '../common/decorators/current-user.decorator';
import { JwtAuthGuard } from '../common/guards/jwt-auth.guard';
import { CreateDmMessageDto } from './dto/create-dm-message.dto';
import { CreateThreadDto } from './dto/create-thread.dto';
import { ListDmMessagesQueryDto } from './dto/list-dm-messages.dto';
import { DmService } from './dm.service';

@ApiTags('dm')
@ApiBearerAuth()
@UseGuards(JwtAuthGuard)
@Controller('dm')
export class DmController {
  constructor(private readonly dmService: DmService) {}

  @Get('threads')
  async listThreads(@CurrentUser() user: CurrentUserShape) {
    return this.dmService.listThreads(user.userId);
  }

  @Post('threads')
  async createThread(@CurrentUser() user: CurrentUserShape, @Body() dto: CreateThreadDto) {
    return this.dmService.createThread(user.userId, dto);
  }

  @Get('threads/:threadId/messages')
  async listMessages(
    @CurrentUser() user: CurrentUserShape,
    @Param('threadId') threadId: string,
    @Query() query: ListDmMessagesQueryDto,
  ) {
    return this.dmService.listMessages(user.userId, threadId, query);
  }

  @Post('threads/:threadId/messages')
  async createMessage(
    @CurrentUser() user: CurrentUserShape,
    @Param('threadId') threadId: string,
    @Body() dto: CreateDmMessageDto,
  ) {
    return this.dmService.createMessage(user.userId, threadId, dto);
  }
}
