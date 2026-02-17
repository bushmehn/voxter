import {
  Body,
  Controller,
  Delete,
  Get,
  Param,
  Patch,
  Post,
  Put,
  Query,
  UploadedFile,
  UseGuards,
  UseInterceptors,
} from '@nestjs/common';
import { ApiBearerAuth, ApiConsumes, ApiTags } from '@nestjs/swagger';
import { FileInterceptor } from '@nestjs/platform-express';
import { CurrentUser, CurrentUserShape } from '../common/decorators/current-user.decorator';
import { JwtAuthGuard } from '../common/guards/jwt-auth.guard';
import { CreateMessageDto } from './dto/create-message.dto';
import { ListMessagesQueryDto } from './dto/list-messages-query.dto';
import { UpdateMessageDto } from './dto/update-message.dto';
import { UploadAttachmentDto } from './dto/upload-attachment.dto';
import { MessagesService } from './messages.service';

@ApiTags('messages')
@ApiBearerAuth()
@UseGuards(JwtAuthGuard)
@Controller()
export class MessagesController {
  constructor(private readonly messagesService: MessagesService) {}

  @Get('channels/:channelId/messages')
  async list(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Query() query: ListMessagesQueryDto,
  ) {
    return this.messagesService.listChannelMessages(user.userId, channelId, query);
  }

  @Post('channels/:channelId/messages')
  async create(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Body() dto: CreateMessageDto,
  ) {
    return this.messagesService.createChannelMessage(user.userId, channelId, dto);
  }

  @Patch('channels/:channelId/messages/:messageId')
  async update(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Param('messageId') messageId: string,
    @Body() dto: UpdateMessageDto,
  ) {
    return this.messagesService.updateChannelMessage(user.userId, channelId, messageId, dto);
  }

  @Delete('channels/:channelId/messages/:messageId')
  async remove(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Param('messageId') messageId: string,
  ) {
    return this.messagesService.deleteChannelMessage(user.userId, channelId, messageId);
  }

  @Put('channels/:channelId/messages/:messageId/reactions/:emoji')
  async toggleReaction(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Param('messageId') messageId: string,
    @Param('emoji') emoji: string,
  ) {
    return this.messagesService.toggleReaction(user.userId, channelId, messageId, emoji);
  }

  @Post('channels/:channelId/typing')
  async typing(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
  ) {
    return this.messagesService.sendTyping(user.userId, channelId);
  }

  @Post('channels/:channelId/attachments')
  @UseInterceptors(FileInterceptor('file'))
  @ApiConsumes('multipart/form-data')
  async upload(
    @CurrentUser() user: CurrentUserShape,
    @Param('channelId') channelId: string,
    @Body() dto: UploadAttachmentDto,
    @UploadedFile() file: Express.Multer.File,
  ) {
    return this.messagesService.createAttachmentMessage(user.userId, channelId, file, dto);
  }
}
