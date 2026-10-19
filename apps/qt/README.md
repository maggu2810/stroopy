# apps/qt/

Implementation: **C++23 + Qt 6.12 (Qt Quick/QML, Material style)**.
Targets: **Linux desktop**, **Android arm64** (later Windows, macOS, iOS).

| | |
|---|---|
| Supported versions | rules **1** · share code — · protocol — |
| Android application id | `de.maggu2810.android.stroopy.qt` |

## Layout

```
src/core/   game core in plain C++23, no Qt (rules v1, game session)
src/app/    Qt Quick app: GameController (C++) + QML UI
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
