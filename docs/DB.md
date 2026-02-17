# Database Design (Prisma + PostgreSQL)

## Core Entities
- `User`: account identity, profile, current presence.
- `RefreshToken`: hashed refresh credentials with rotation chain.
- `Guild`: server container owned by a user.
- `GuildMember`: membership join table.
- `Role`: guild role with permission bitmask.
- `GuildMemberRole`: role assignments per member.
- `Channel`: text or voice channel under a guild.
- `ChannelPermissionOverwrite`: optional allow/deny masks by role or user.
- `Invite`: invite code records with usage and expiration.
- `Message`: channel/DM message with soft-delete/edit timestamps and tsvector.
- `Attachment`: uploaded file metadata linked to message.
- `Reaction`: per-user per-message emoji reaction.
- `DMThread` + `DMThreadMember`: direct/group DM rooms.
- `VoiceState`: current per-user voice state per guild channel.

## Key Indexes
- `User.email` unique index.
- `GuildMember(guildId,userId)` PK and `userId` index for membership lookups.
- `Role(guildId,position)` index for role ordering.
- `Channel(guildId,position)` index for channel ordering.
- `Invite.code` unique and `Invite.guildId` index.
- `Message(channelId,createdAt DESC)` and `Message(dmThreadId,createdAt DESC)` for pagination.
- `Message.searchVector` GIN index for full-text search.
- `Reaction(messageId,userId,emoji)` unique to enforce reaction uniqueness.
- `VoiceState(guildId,userId)` unique to ensure one active voice state per guild user.

## Full-Text Search
- `Message.searchVector` column is maintained by trigger:
  - `message_search_vector_trigger`
- Query path uses:
  - `plainto_tsquery('simple', :search)`
  - `searchVector @@ tsquery`

## Migration
- Initial migration file:
  - `server/prisma/migrations/202602120001_init/migration.sql`
- Prisma schema:
  - `server/prisma/schema.prisma`
