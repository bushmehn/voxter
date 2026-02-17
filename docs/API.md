# API and WS Contracts

## Versioning
- REST API versioning: URL version omitted in MVP, semantic contract tracked by backend release.
- WebSocket envelope version: `v = "1.0"`.
- WS payload envelope:
  - `v`: protocol version string
  - `t`: event type
  - `d`: event data object

## REST Endpoints (MVP)

### Auth
- `POST /auth/register`
- `POST /auth/login`
- `POST /auth/refresh`
- `GET /auth/me`
- `POST /auth/logout`
- `POST /auth/logout-all`

### Users
- `PATCH /users/me`

### Guilds / Roles / Channels / Invites
- `GET /guilds/me`
- `POST /guilds`
- `GET /guilds/:guildId/channels`
- `POST /guilds/:guildId/channels`
- `POST /guilds/:guildId/invites`
- `POST /guilds/join/:code`
- `GET /guilds/:guildId/roles`
- `POST /guilds/:guildId/roles`
- `PATCH /guilds/:guildId/roles/:roleId`
- `POST /guilds/:guildId/members/:memberId/roles/:roleId`
- `POST /guilds/:guildId/channels/:channelId/overwrites`

### Messages / Attachments
- `GET /channels/:channelId/messages`
- `POST /channels/:channelId/messages`
- `PATCH /channels/:channelId/messages/:messageId`
- `DELETE /channels/:channelId/messages/:messageId`
- `PUT /channels/:channelId/messages/:messageId/reactions/:emoji`
- `POST /channels/:channelId/typing`
- `POST /channels/:channelId/attachments`

### DM
- `GET /dm/threads`
- `POST /dm/threads`
- `GET /dm/threads/:threadId/messages`
- `POST /dm/threads/:threadId/messages`

### Presence
- `PATCH /presence/me`
- `GET /presence/guild/:guildId`

### Voice
- `POST /voice/join`
- `POST /voice/sfu-token`
  - body: `{ guildId, channelId }`
  - response: `{ provider, url, roomName, token, identity, ttlSeconds }`
- `POST /voice/leave`
- `PATCH /voice/state`
- `GET /voice/channel/:channelId`

## WS Incoming Events (Client -> Server)
- `subscribe_channels`
  - `{ channelIds: string[] }`
- `subscribe_dm_threads`
  - `{ threadIds: string[] }`
- `typing`
  - `{ channelId: string }`
- `presence_set`
  - `{ status: "ONLINE"|"IDLE"|"DND"|"OFFLINE" }`
- `voice_signal`
  - `{ channelId: string, targetUserId: string, signal: object }`
- `voice_audio`
  - `{ guildId: string, channelId: string, pcm: base64, sampleRate: number, channels: number, sampleFormat: "s16le" }`
- `voice_speaking`
  - `{ guildId: string, channelId: string, speaking: boolean }`
- `ping`
  - `{}`

## WS Outgoing Events (Server -> Client)
- `hello`
- `pong`
- `message_create`
- `message_update`
- `message_delete`
- `message_reaction_add`
- `message_reaction_remove`
- `typing`
- `presence_update`
- `channel_create`
- `guild_member_join`
- `dm_thread_create`
- `dm_message_create`
- `voice_state_update`
- `voice_signal`
- `voice_audio`
- `voice_speaking_update`

All outgoing events follow envelope `{ v: "1.0", t: eventName, d: payload }`.
