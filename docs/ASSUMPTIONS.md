# Voxter MVP Assumptions

1. Target architecture is a desktop-first monorepo: Qt 6 client + NestJS backend.
2. Primary deployment mode is single-region, single backend replica for MVP.
3. PostgreSQL is the source of truth; Redis is used for transient presence state.
4. WS gateway is plain WebSocket (`/gateway`) with event envelope `{ v, t, d }`.
5. Permission model is role-bitmask based, with optional channel overwrites.
6. JWT access token TTL is short (15m), refresh token TTL is 7 days with rotation.
7. Client secure token storage uses obfuscated `QSettings` fallback; production should prefer OS keychain integration.
8. Message content is sanitized server-side before persistence.
9. Voice MVP uses LiveKit SFU with native LiveKit C++ client runtime in `NativeVoiceEngine`.
10. Self-hosted LiveKit SFU is provisioned in docker and `/voice/sfu-token` is required before joining media.
11. Attachment storage defaults to local filesystem in dev; MinIO is optional via env flag.
12. CI validates backend tests and client buildability; full end-to-end media QA is manual.
13. Internationalization is out of MVP scope; UI language is English for now.
