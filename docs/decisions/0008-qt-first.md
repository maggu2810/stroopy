# 0008: Qt is the first implementation

Status: accepted (2026-10-19)

> **Note (2026-10-23):** the web app is publicly hosted on GitHub Pages
> (decision 0013). That is a website, not store distribution, so this
> decision still holds.

## Context
The maintainer wants the game on their Linux (KDE) desktop and Android phone
first. Public store distribution (F-Droid, Play) is not planned for now: the
app is meant for the maintainer and friends.

## Decision
- The first implementation is `apps/qt/` (decision 0006). It targets
  **Linux desktop and Android (arm64)** from the start.
- `apps/compose-android/` (decision 0003) is postponed. It stays a valid
  future implementation.
- Distribution: APK and desktop builds are shared directly. F-Droid
  buildability is not a requirement.

## Consequences
- A Qt Android APK is larger than a Compose one. Some Android integrations
  (share sheet, deep links, NSD) need small JNI/Java bridges.
- Because of decision 0005, nothing in `docs/spec/` depends on this choice.
