# Voxter MVP Limitations

1. Voice transport is LiveKit SFU via native LiveKit C++ client; custom ICE/network interface policy in user settings is not implemented yet.
2. LiveKit C++ SDK is bundled from `client/third_party`; upgrades require manual SDK refresh and binary compatibility checks.
3. Advanced audio controls (per-user volume, noise suppression profile tuning, echo diagnostics) are minimal.
4. Client secure storage fallback is obfuscation, not strong cryptographic at-rest protection.
5. WS gateway uses in-memory subscriptions; horizontal scaling needs shared state or broker fan-out.
6. Message search is implemented with PostgreSQL full-text query and index, without advanced ranking or stemming configuration.
7. Attachments use single-file upload endpoint; resumable and chunked upload are not implemented.
8. No end-to-end encryption for messages or voice in MVP.
9. Role hierarchy conflict resolution is minimal and focused on core permission checks.
10. Channel categories and drag-and-drop ordering UX are simplified for MVP.
11. DM unread counters, push notifications, and offline sync conflict resolution are not implemented.
12. Automated e2e tests cover service availability flow only; richer cross-service scenario coverage is partial.
