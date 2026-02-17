# Security Notes

## Implemented Controls
1. Password hashing with `argon2`.
2. JWT access token + rotating refresh token model.
3. Refresh token server-side hashing (refresh secret not stored in plain text).
4. DTO validation (`class-validator`) with whitelist and forbid unknown fields.
5. Global rate limiting via Nest throttler.
6. Helmet security headers.
7. CORS explicit configuration via env.
8. Message content sanitization before persistence.
9. Permission checks for guild/channel operations.
10. Soft-delete semantics for messages.
11. LiveKit SFU access token issuance is server-side only (API key/secret never sent to client).

## Client Token Storage
- Current implementation uses obfuscated `QSettings` fallback.
- This avoids plain text but is weaker than OS keychain.
- Production recommendation:
  - integrate QtKeychain (Windows Credential Manager / macOS Keychain / libsecret).

## Further Hardening (Next Iteration)
1. Replace static TURN credentials with time-limited credentials.
2. Add audit logs for admin and moderation actions.
3. Add brute-force heuristics and IP reputation checks.
4. Add CSP-like controls for any rendered rich content source.
5. Add secrets vault integration for runtime secrets.
6. Add automated SAST/DAST and dependency vulnerability scanning.
7. Use long random LiveKit API secret and terminate TLS (WSS/HTTPS) in production.
