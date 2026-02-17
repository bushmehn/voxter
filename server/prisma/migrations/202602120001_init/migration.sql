-- CreateEnum
CREATE TYPE "ChannelType" AS ENUM ('TEXT', 'VOICE');

-- CreateEnum
CREATE TYPE "PresenceStatus" AS ENUM ('ONLINE', 'IDLE', 'DND', 'OFFLINE');

CREATE TABLE "User" (
  "id" TEXT NOT NULL,
  "email" TEXT NOT NULL,
  "passwordHash" TEXT NOT NULL,
  "displayName" TEXT NOT NULL,
  "avatarUrl" TEXT,
  "presence" "PresenceStatus" NOT NULL DEFAULT 'OFFLINE',
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "User_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "RefreshToken" (
  "id" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "tokenHash" TEXT NOT NULL,
  "expiresAt" TIMESTAMP(3) NOT NULL,
  "revokedAt" TIMESTAMP(3),
  "rotatedFromId" TEXT,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "RefreshToken_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "Guild" (
  "id" TEXT NOT NULL,
  "name" TEXT NOT NULL,
  "ownerId" TEXT NOT NULL,
  "iconUrl" TEXT,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "Guild_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "GuildMember" (
  "guildId" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "nickname" TEXT,
  "joinedAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "GuildMember_pkey" PRIMARY KEY ("guildId","userId")
);

CREATE TABLE "Role" (
  "id" TEXT NOT NULL,
  "guildId" TEXT NOT NULL,
  "name" TEXT NOT NULL,
  "color" TEXT,
  "position" INTEGER NOT NULL DEFAULT 0,
  "permissions" BIGINT NOT NULL,
  "isDefault" BOOLEAN NOT NULL DEFAULT false,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "Role_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "GuildMemberRole" (
  "guildId" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "roleId" TEXT NOT NULL,
  CONSTRAINT "GuildMemberRole_pkey" PRIMARY KEY ("guildId","userId","roleId")
);

CREATE TABLE "Channel" (
  "id" TEXT NOT NULL,
  "guildId" TEXT NOT NULL,
  "name" TEXT NOT NULL,
  "type" "ChannelType" NOT NULL,
  "position" INTEGER NOT NULL DEFAULT 0,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "Channel_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "ChannelPermissionOverwrite" (
  "id" TEXT NOT NULL,
  "channelId" TEXT NOT NULL,
  "roleId" TEXT,
  "userId" TEXT,
  "allow" BIGINT NOT NULL DEFAULT 0,
  "deny" BIGINT NOT NULL DEFAULT 0,
  CONSTRAINT "ChannelPermissionOverwrite_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "Invite" (
  "id" TEXT NOT NULL,
  "code" TEXT NOT NULL,
  "guildId" TEXT NOT NULL,
  "channelId" TEXT,
  "creatorId" TEXT NOT NULL,
  "expiresAt" TIMESTAMP(3),
  "maxUses" INTEGER,
  "uses" INTEGER NOT NULL DEFAULT 0,
  "revoked" BOOLEAN NOT NULL DEFAULT false,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "Invite_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "DMThread" (
  "id" TEXT NOT NULL,
  "name" TEXT,
  "isGroup" BOOLEAN NOT NULL DEFAULT false,
  "ownerId" TEXT,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "DMThread_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "DMThreadMember" (
  "threadId" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "joinedAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "DMThreadMember_pkey" PRIMARY KEY ("threadId","userId")
);

CREATE TABLE "Message" (
  "id" TEXT NOT NULL,
  "channelId" TEXT,
  "dmThreadId" TEXT,
  "authorId" TEXT NOT NULL,
  "content" TEXT NOT NULL,
  "editedAt" TIMESTAMP(3),
  "deletedAt" TIMESTAMP(3),
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  "searchVector" tsvector,
  CONSTRAINT "Message_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "Attachment" (
  "id" TEXT NOT NULL,
  "messageId" TEXT NOT NULL,
  "fileName" TEXT NOT NULL,
  "mimeType" TEXT NOT NULL,
  "size" INTEGER NOT NULL,
  "url" TEXT NOT NULL,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "Attachment_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "Reaction" (
  "id" TEXT NOT NULL,
  "messageId" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "emoji" TEXT NOT NULL,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "Reaction_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "VoiceState" (
  "id" TEXT NOT NULL,
  "guildId" TEXT NOT NULL,
  "channelId" TEXT NOT NULL,
  "userId" TEXT NOT NULL,
  "muted" BOOLEAN NOT NULL DEFAULT false,
  "deafened" BOOLEAN NOT NULL DEFAULT false,
  "joinedAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "VoiceState_pkey" PRIMARY KEY ("id")
);

CREATE UNIQUE INDEX "User_email_key" ON "User"("email");
CREATE INDEX "RefreshToken_userId_idx" ON "RefreshToken"("userId");
CREATE INDEX "RefreshToken_expiresAt_idx" ON "RefreshToken"("expiresAt");
CREATE INDEX "GuildMember_userId_idx" ON "GuildMember"("userId");
CREATE INDEX "Role_guildId_position_idx" ON "Role"("guildId", "position");
CREATE INDEX "GuildMemberRole_roleId_idx" ON "GuildMemberRole"("roleId");
CREATE INDEX "Channel_guildId_position_idx" ON "Channel"("guildId", "position");
CREATE INDEX "ChannelPermissionOverwrite_channelId_idx" ON "ChannelPermissionOverwrite"("channelId");
CREATE INDEX "ChannelPermissionOverwrite_roleId_idx" ON "ChannelPermissionOverwrite"("roleId");
CREATE INDEX "ChannelPermissionOverwrite_userId_idx" ON "ChannelPermissionOverwrite"("userId");
CREATE UNIQUE INDEX "Invite_code_key" ON "Invite"("code");
CREATE INDEX "Invite_guildId_idx" ON "Invite"("guildId");
CREATE INDEX "DMThreadMember_userId_idx" ON "DMThreadMember"("userId");
CREATE INDEX "Message_channelId_createdAt_idx" ON "Message"("channelId", "createdAt" DESC);
CREATE INDEX "Message_dmThreadId_createdAt_idx" ON "Message"("dmThreadId", "createdAt" DESC);
CREATE INDEX "Message_authorId_idx" ON "Message"("authorId");
CREATE INDEX "Attachment_messageId_idx" ON "Attachment"("messageId");
CREATE UNIQUE INDEX "Reaction_messageId_userId_emoji_key" ON "Reaction"("messageId", "userId", "emoji");
CREATE INDEX "Reaction_messageId_idx" ON "Reaction"("messageId");
CREATE UNIQUE INDEX "VoiceState_guildId_userId_key" ON "VoiceState"("guildId", "userId");
CREATE INDEX "VoiceState_channelId_idx" ON "VoiceState"("channelId");
CREATE INDEX "Message_searchVector_idx" ON "Message" USING GIN ("searchVector");

ALTER TABLE "RefreshToken" ADD CONSTRAINT "RefreshToken_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Guild" ADD CONSTRAINT "Guild_ownerId_fkey" FOREIGN KEY ("ownerId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "GuildMember" ADD CONSTRAINT "GuildMember_guildId_fkey" FOREIGN KEY ("guildId") REFERENCES "Guild"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "GuildMember" ADD CONSTRAINT "GuildMember_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Role" ADD CONSTRAINT "Role_guildId_fkey" FOREIGN KEY ("guildId") REFERENCES "Guild"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "GuildMemberRole" ADD CONSTRAINT "GuildMemberRole_guildId_userId_fkey" FOREIGN KEY ("guildId", "userId") REFERENCES "GuildMember"("guildId", "userId") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "GuildMemberRole" ADD CONSTRAINT "GuildMemberRole_roleId_fkey" FOREIGN KEY ("roleId") REFERENCES "Role"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Channel" ADD CONSTRAINT "Channel_guildId_fkey" FOREIGN KEY ("guildId") REFERENCES "Guild"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "ChannelPermissionOverwrite" ADD CONSTRAINT "ChannelPermissionOverwrite_channelId_fkey" FOREIGN KEY ("channelId") REFERENCES "Channel"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "ChannelPermissionOverwrite" ADD CONSTRAINT "ChannelPermissionOverwrite_roleId_fkey" FOREIGN KEY ("roleId") REFERENCES "Role"("id") ON DELETE SET NULL ON UPDATE CASCADE;
ALTER TABLE "ChannelPermissionOverwrite" ADD CONSTRAINT "ChannelPermissionOverwrite_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE SET NULL ON UPDATE CASCADE;
ALTER TABLE "Invite" ADD CONSTRAINT "Invite_guildId_fkey" FOREIGN KEY ("guildId") REFERENCES "Guild"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Invite" ADD CONSTRAINT "Invite_channelId_fkey" FOREIGN KEY ("channelId") REFERENCES "Channel"("id") ON DELETE SET NULL ON UPDATE CASCADE;
ALTER TABLE "Invite" ADD CONSTRAINT "Invite_creatorId_fkey" FOREIGN KEY ("creatorId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "DMThread" ADD CONSTRAINT "DMThread_ownerId_fkey" FOREIGN KEY ("ownerId") REFERENCES "User"("id") ON DELETE SET NULL ON UPDATE CASCADE;
ALTER TABLE "DMThreadMember" ADD CONSTRAINT "DMThreadMember_threadId_fkey" FOREIGN KEY ("threadId") REFERENCES "DMThread"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "DMThreadMember" ADD CONSTRAINT "DMThreadMember_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Message" ADD CONSTRAINT "Message_channelId_fkey" FOREIGN KEY ("channelId") REFERENCES "Channel"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Message" ADD CONSTRAINT "Message_dmThreadId_fkey" FOREIGN KEY ("dmThreadId") REFERENCES "DMThread"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Message" ADD CONSTRAINT "Message_authorId_fkey" FOREIGN KEY ("authorId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Attachment" ADD CONSTRAINT "Attachment_messageId_fkey" FOREIGN KEY ("messageId") REFERENCES "Message"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Reaction" ADD CONSTRAINT "Reaction_messageId_fkey" FOREIGN KEY ("messageId") REFERENCES "Message"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Reaction" ADD CONSTRAINT "Reaction_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "VoiceState" ADD CONSTRAINT "VoiceState_guildId_fkey" FOREIGN KEY ("guildId") REFERENCES "Guild"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "VoiceState" ADD CONSTRAINT "VoiceState_channelId_fkey" FOREIGN KEY ("channelId") REFERENCES "Channel"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "VoiceState" ADD CONSTRAINT "VoiceState_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;

CREATE OR REPLACE FUNCTION message_search_vector_trigger() RETURNS trigger AS $$
BEGIN
  NEW."searchVector" := to_tsvector('simple', coalesce(NEW."content", ''));
  RETURN NEW;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER message_search_vector_update
BEFORE INSERT OR UPDATE ON "Message"
FOR EACH ROW
EXECUTE FUNCTION message_search_vector_trigger();
