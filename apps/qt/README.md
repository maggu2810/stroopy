# apps/qt/

Implementation: **C++23 + Qt 6.12 (Qt Quick/QML, Material style)**.
Targets: **Linux desktop**, **Android arm64** (later Windows, macOS, iOS).

| | |
|---|---|
| Supported versions | rules **1** · share code **1** · backup **1** · protocol **1** (Nostr) |
| Android application id | `de.maggu2810.android.stroopy.qt` |

## Features

- All four modes of rules v1, with 3-2-1-GO countdown and timers
- Highscores per mode (top 10); share or replay every entry
- Share codes (`SYG1-…`, copied to the clipboard as a message) and *Enter code*
  to play a received sequence as a challenge against the sharer's score
- Settings: language (follow system / English / Deutsch, applied live),
  colour-blind palette, vibration on failure (Android)
- Portrait and landscape layouts on every screen (phones, tablets, desktop, TV)
- Keyboard, gamepad (Android) and TV-remote input; focus navigation in menus;
  key hints appear after the first key press; F11 fullscreen on desktop
- Android TV: TV launcher entry with banner, touchscreen not required
- Backup code (`SYB1-…`): export/import of highscores and settings, also to/from the web app
- Live multiplayer over public Nostr relays: create/join a room (`SYR1-…`), the host
  starts, everyone plays the same sequence and sees the others' progress; plays
  together with the web app
- In-app Help page: modes, controls, sharing, multiplayer, about

## Layout

```
src/core/   game core in plain C++23, no Qt (rules, share code, backup, room codes, game session)
src/net/    Nostr: events and signatures (libsecp256k1), protocol messages, relay pool (Qt WebSockets)
src/app/    Qt Quick app: C++ (GameController, Highscores, Sharing, Backup, Multiplayer,
            AppSettings) + QML UI
src/app/i18n/  translations (Stroopy_<lang>.ts); refresh with the update_translations target
android/    Android package sources: manifest (TV, vibration), StroopyActivity (gamepad
            buttons → JNI), TV banner (assets/artwork/banner.svg → tools/render-assets.sh)
tests/      conformance tests (rules, share code, backup, protocol), a headless UI test (tst_ui)
            and interop_nostr, a live driver for apps/web's `bun run test:interop`
```

## Build

Prerequisites: `tools/setup-qt.sh` (Qt 6.12.0 into `~/Qt-aqt`), CMake ≥ 4,
Ninja. CMake downloads libsecp256k1 (MIT) and, for Android, KDAB's prebuilt
OpenSSL 3 (Apache-2.0, needed for `wss://`), both pinned by hash. For Android also: Android SDK in `~/Android/Sdk` with NDK
`27.2.12479018` and platform `android-37.0`.

```sh
# Linux
cmake --preset linux && cmake --build --preset linux
ctest --preset linux                     # conformance + UI tests (UI test renders in software)
tools/run-ui-test-headless.sh /tmp/shots # UI test in a private headless KWin (OpenGL) + screenshots
                                         # (STROOPY_UI_NETWORK=1 also opens a room on the relays)
build/linux/src/app/stroopy

# Android (debug-signed APK)
cmake --preset android && cmake --build --preset android
adb install -r build/android/src/app/android-build/stroopy.apk
```
