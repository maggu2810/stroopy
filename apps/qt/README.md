# apps/qt/

Implementation: **C++23 + Qt 6.12 (Qt Quick/QML, Material style)**.
Targets: **Linux desktop**, **Android arm64** (later Windows, macOS, iOS).

| | |
|---|---|
| Supported versions | rules **1** · share code **1** · protocol — |
| Android application id | `de.maggu2810.android.stroopy.qt` |

## Features

- All four modes of rules v1, with 3-2-1-GO countdown and timers
- Highscores per mode (top 10); share or replay every entry
- Share codes (`SY1-…`, copied to the clipboard as a message) and *Enter code*
  to play a received sequence as a challenge against the sharer's score
- Settings: language (follow system / English / Deutsch, applied live),
  colour-blind palette, vibration (setting only; haptics not wired yet)

## Layout

```
src/core/   game core in plain C++23, no Qt (rules v1, share code v1, game session)
src/app/    Qt Quick app: C++ (GameController, Highscores, Sharing, AppSettings) + QML UI
src/app/i18n/  translations (Stroopy_<lang>.ts); refresh with the update_translations target
tests/      Qt Test conformance suite for docs/spec/test-vectors/rules/v1
```

## Build

Prerequisites: `tools/setup-qt.sh` (Qt 6.12.0 into `~/Qt-aqt`), CMake ≥ 4,
Ninja. For Android also: Android SDK in `~/Android/Sdk` with NDK
`27.2.12479018` and platform `android-37.0`.

```sh
# Linux
cmake --preset linux && cmake --build --preset linux
ctest --preset linux                     # conformance tests
build/linux/src/app/stroopy

# Android (debug-signed APK)
cmake --preset android && cmake --build --preset android
adb install -r build/android/src/app/android-build/stroopy.apk
```
