# Voxter

Voxter is a desktop Discord-like MVP built as a monorepo:
- `client/`: Qt 6 (QML + C++) desktop app.
- `server/`: NestJS + Prisma + PostgreSQL + Redis backend.
- `docker/`: local infrastructure (`api`, `db`, `redis`, `minio`, `livekit`).
- `docs/`: architecture, API, DB, security, voice, plan, limitations.

## Repository Layout

```text
.
+-- client/
+-- server/
+-- docker/
+-- docs/
L-- .github/workflows/
```

## Quick Start

### 1) Infrastructure + API

```bash
cd docker
cp .env.example .env
docker compose up --build -d
```

API will be available at `http://localhost:4000`.
Swagger UI: `http://localhost:4000/swagger`.
LiveKit WS endpoint: `ws://localhost:7880`.

Important: change `LIVEKIT_API_SECRET` in `docker/.env` for any non-local deployment.

### 2) Backend Local Development (optional)

```bash
cd server
cp .env.example .env
npm install
npm run prisma:generate
npm run prisma:dev
npm run start:dev
```

### 3) Qt Client Build

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
```

Executable target: `voxter_client`.

If backend runs elsewhere:

```bash
set VOXTER_API_URL=http://<host>:4000
```

Or place `voxter_client.ini` next to `voxter_client.exe`:

```ini
[network]
api_url=http://<host>:4000
```

### 4) Tests

Backend:

```bash
cd server
npm run test:unit
npm run test:integration
npm run test:e2e
```

Client:

```bash
ctest --test-dir build/client --output-on-failure
```

## MVP Feature Coverage

- Auth with refresh token rotation.
- Guilds/channels/roles/invites.
- Realtime channel messages over WS.
- Message history, edit/delete, reactions, typing.
- Attachment upload endpoint.
- Presence updates.
- DM threads and messages.
- Voice state + LiveKit SFU media transport (native LiveKit C++ client).

## Operational Notes

- Read `docs/ASSUMPTIONS.md` and `docs/LIMITATIONS.md` before productionizing.
- Security details are in `docs/SECURITY.md`.
- Voice architecture is in `docs/VOICE.md`.
- Versions and source links are in `docs/VERSIONS.md`.
