# Voice MVP Architecture

## Selected Approach
- Option selected: **LiveKit SFU + native LiveKit C++ client**.
- Rationale: production-like audio quality/latency and scalable multiparty voice without mesh or PCM relay.
- Runtime mode: **LiveKit-only** (no WebEngine bridge, no PCM relay fallback).

## Components
- Native voice engine:
  - `client/src/services/NativeVoiceEngine.h`
  - `client/src/services/NativeVoiceEngine.cpp`
- C++ VM orchestration:
  - `client/src/viewmodels/VoiceViewModel.cpp`
- Token issuance backend:
  - `POST /voice/sfu-token`
  - `server/src/voice/voice.service.ts`
- Bundled client SDK:
  - `client/third_party/livekit-sdk-0.3.1/livekit-sdk-windows-x64-0.3.1`

## Runtime Flow
1. User joins voice channel via REST `POST /voice/join`.
2. Client requests SFU credentials via REST `POST /voice/sfu-token`.
3. `NativeVoiceEngine` connects to LiveKit room with URL + token.
4. Local mic is captured via `QAudioSource` and published as LiveKit local audio track.
5. Remote participant tracks are subscribed and played via `livekit::AudioStream` + `QAudioSink`.
6. Active speakers are propagated to app store and shown in channel/member UI.

## LiveKit Environment
- Env separation:
  - `LIVEKIT_WS_URL` (client-facing URL in token response, e.g. `ws://localhost:7880`)
  - `LIVEKIT_REST_URL` (server-to-livekit control URL, e.g. `http://livekit:7880`)
  - `LIVEKIT_API_KEY`
  - `LIVEKIT_API_SECRET`
  - `LIVEKIT_ROOM_PREFIX`

## Known Constraints
- Native client currently uses default ICE/network behavior from LiveKit SDK (no custom per-interface routing policy in app settings).
- End-to-end media encryption beyond SRTP defaults is not configured in MVP.
