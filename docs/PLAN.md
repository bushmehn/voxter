# Voxter Implementation Plan

## Phase 1: Monorepo Foundation
- Create monorepo layout: `client/`, `server/`, `docker/`, `docs/`, CI.
- Wire root CMake to Qt client subproject.
- Add root-level README and environment templates.

## Phase 2: Backend Core (NestJS)
- Bootstrap Nest app with:
  - Config validation.
  - ValidationPipe.
  - Helmet + CORS.
  - Throttling guard.
  - Swagger/OpenAPI.
- Add Prisma + PostgreSQL schema and migration.
- Add Redis service wrapper.

## Phase 3: Authentication & Security
- Implement `auth/register`, `auth/login`, `auth/refresh`, `auth/me`, logout flows.
- Use argon2 for password hashing.
- Implement refresh token rotation with server-side hashed refresh secret.
- Add JWT guard and strategy.

## Phase 4: Guilds, Roles, Channels, Invites
- Implement guild creation and guild membership listing.
- Create default roles and channel seeds (`general`, `voice`) on guild creation.
- Implement role CRUD and role assignment.
- Implement channel creation and channel overwrite storage.
- Implement invite creation and join-by-code.

## Phase 5: Messaging & Realtime
- Implement channel messages:
  - history with pagination
  - create / update / delete
  - reactions
  - typing
- Implement attachment upload endpoint (local + MinIO drivers).
- Implement PostgreSQL full-text search path for message lookup.
- Broadcast WS events: create/update/delete/reaction/typing.

## Phase 6: DM, Presence, Voice State
- Implement DM threads and DM messages.
- Implement presence update endpoint + realtime broadcast.
- Implement voice state endpoints (join/leave/mute/deafen) and participant listing.
- Add WS voice events (`voice_audio`, `voice_speaking`) and voice state updates.
- Add self-hosted LiveKit SFU service in docker and token issuance endpoint (`/voice/sfu-token`).

## Phase 7: Qt Client (QML + C++)
- Implement architecture layers:
  - Services: REST, WS, secure storage, voice bridge.
  - Store: reactive app state.
  - ViewModels: auth, guild/channel, chat, voice.
- Implement QML Discord-like layout with:
  - server rail
  - channel panel
  - chat area
  - members panel
  - voice dock
- Add login/register flow and session bootstrap.
- Add realtime chat updates through WS gateway.
- Add native LiveKit C++ client runtime and direct audio settings sync in `NativeVoiceEngine`.

## Phase 8: Testing & CI
- Backend:
  - unit test for permissions
  - integration tests for auth/messages services
  - e2e service health path
- Client:
  - QtTest for store behavior.
- CI pipeline:
  - backend install/generate/test
  - client configure/build/test

## Phase 9: Documentation & Operational Readiness
- Document DB schema and indexes.
- Document REST + WS event contracts and versioning.
- Document voice MVP architecture and security.
- Document limitations and assumptions.
