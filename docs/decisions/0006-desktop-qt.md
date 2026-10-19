# 0006: Desktop: Qt 6

Status: accepted (2026-10-19)

## Context
Desktop targets are Linux, Windows and macOS. The maintainer runs KDE Plasma,
which is built on Qt. Candidates: one toolkit per OS (GTK/WinUI/AppKit),
Qt, Compose Desktop (JVM), Rust + Slint/egui, Electron/Tauri.

## Decision
The desktop implementation uses **Qt 6** with **Qt Quick/QML** for the UI
and **C++** for logic, built with **CMake**. It lives in `apps/qt/`.

## Rationale
- Native on KDE. It looks and behaves well on GNOME, Windows and macOS too.
- One codebase for all three desktop OSes. It can also target Android and
  iOS, which would give an alternative mobile implementation.
- Qt has modules for what we need: Qt Network (TCP, mDNS via
  third-party/KDNSSD, or the planned Qt mDNS support), Qt WebSockets (Nostr),
  Qt Bluetooth (BLE), Qt Multimedia (QR scanning via camera).
- Licence: Qt is available under LGPL-3.0/GPL-3.0, which is compatible with
  our GPL-3.0-or-later.
- Optional: KDE **Kirigami** on top of Qt Quick for convergent desktop/mobile
  UI and Plasma integration. To be decided when `apps/qt/` starts.

## Consequences
- GTK is not planned. A GTK app looks foreign on KDE, and a Qt app is
  acceptable on GNOME.
- The game core is written in C++ against `docs/spec/` and the conformance
  vectors (decision 0005). It does not wrap the Kotlin core.
