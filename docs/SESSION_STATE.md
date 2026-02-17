# Voxter Session State

Last updated: February 16, 2026

## Current status
- Backend is running via Docker and API/Gateway routes are available.
- Qt client layout is in Discord-like style with 3 columns.
- Left side has server icons and a `+` dialog with tabs `Create` and `Join`.
- `Direct Messages` block is removed from the channel column.
- Server members are shown in the right panel.
- Voice participants are shown under each voice channel in the channel list.
- Joining voice is by clicking a voice channel.
- Voice controls are in the left channel column (`Leave`, `Mute`, `Deafen`).

## Important recent fixes
- Added QML module registration for `LeftProfileDock.qml` in `client/CMakeLists.txt`.
- Prevented voice request spam with in-flight guards and participant refresh throttling.
- Fixed stale voice participant display when rapidly switching channels.
- Microphone is requested/used only while in a voice session; released on leave.
- Presence now updates to `ONLINE` on auth bootstrap/login and updates in realtime from `presence_update`.
- Added separate bottom profile dock overlay across the two left columns, with bottom insets so content is not hidden.

## Files touched recently
- `client/qml/screens/MainScreen.qml`
- `client/qml/components/ServerList.qml`
- `client/qml/components/ChannelList.qml`
- `client/qml/components/LeftProfileDock.qml`
- `client/src/viewmodels/VoiceViewModel.cpp`
- `client/src/viewmodels/VoiceViewModel.h`
- `client/src/viewmodels/AuthViewModel.cpp`
- `client/src/app/Application.cpp`
- `client/src/store/AppStore.cpp`
- `client/src/store/AppStore.h`
- `client/CMakeLists.txt`

## Rebuild/run notes
- In Qt Creator run `Reconfigure Project` after QML/CMake changes.
- Then `Clean` + `Rebuild`.
- If `LeftProfileDock is not a type` appears, verify `client/CMakeLists.txt` contains `qml/components/LeftProfileDock.qml`.
