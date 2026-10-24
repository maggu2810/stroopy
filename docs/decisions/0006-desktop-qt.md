# 0006: Qt 6 for desktop and Android

Status: accepted (2026-10-19)

## Context
Targets are Linux desktop and Android first, then Windows and macOS. The
maintainer runs KDE Plasma, which is built on Qt. Candidates: one toolkit
per OS (GTK/WinUI/AppKit), Qt, Compose Multiplatform (JVM), Rust +
Slint/egui, Electron/Tauri.

## Decision
`apps/qt/` uses **Qt 6** with **Qt Quick/QML** for the UI and **C++** for
logic, built with **CMake**. It targets Linux desktop and Android (arm64),
and can later target Windows, macOS and iOS.

## Rationale
- Native on KDE. It looks and behaves well on GNOME, Windows and macOS too.
- One codebase for all desktop OSes and Android.
- Qt has modules for what we need: Qt WebSockets for Nostr (decision 0012),
  and later Qt Network (LAN), Qt Bluetooth (BLE) and Qt Multimedia (QR
  scanning).
- Licence: Qt is available under LGPL-3.0/GPL-3.0, which is compatible with
  our GPL-3.0-or-later.

## Consequences
- GTK is not planned. A GTK app looks foreign on KDE, and a Qt app is
  acceptable on GNOME.
- The UI uses plain Qt Quick. Whether to add KDE **Kirigami** is an open
  decision.
- Android integrations without a Qt API (vibration, gamepad buttons, share
  sheet, links) use small JNI/Java bridges.
- The game core is written in C++ against `docs/spec/` and the conformance
  vectors (decision 0005).
