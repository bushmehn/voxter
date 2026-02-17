# Voxter Smoke Checklist

## Backend + Infra
- [ ] `docker compose up --build -d` from `docker/` succeeds.
- [ ] `http://localhost:4000/swagger` opens.
- [ ] `ws://localhost:7880` accepts LiveKit websocket connection.
- [ ] `POST /auth/register` returns `accessToken`, `refreshToken`, `user`.
- [ ] `POST /auth/login` works with created user.
- [ ] `GET /guilds/me` with bearer token returns array.

## Core Chat Flow
- [ ] In client, login/register succeeds.
- [ ] Create guild from left server rail.
- [ ] Create text channel from channel panel.
- [ ] Send message from composer.
- [ ] Open second client session and confirm realtime message delivery.
- [ ] Edit/delete/reaction actions propagate realtime.

## DM Flow
- [ ] Create DM thread.
- [ ] Send DM message.
- [ ] Confirm realtime DM event delivery in second client.

## Presence
- [ ] Connect two clients and verify `presence_update` events via WS.

## Voice MVP
- [ ] Select voice channel and join from voice dock.
- [ ] Participant list updates in right panel.
- [ ] Mute/deafen toggles update state.
- [ ] `POST /voice/sfu-token` returns `url`, `roomName`, and `token`.
- [ ] LiveKit room join succeeds with issued token.
- [ ] Speaking highlight (`voice_speaking_update`) appears for active speaker.

## Attachments
- [ ] Attach local file in composer.
- [ ] Message with attachment URL appears in chat timeline.

## Security Baseline
- [ ] Confirm password hash stored (not plain text).
- [ ] Confirm refresh endpoint rotates refresh token.
- [ ] Confirm unauthorized calls return 401.
