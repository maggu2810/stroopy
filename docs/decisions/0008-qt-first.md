# 0008: Qt is the first implementation; direct distribution

Status: accepted (2026-10-19)

## Context
The maintainer wants the game on their Linux (KDE) desktop and Android phone
first. The app is meant for the maintainer, family and friends; public store
distribution is not a goal for now.

## Decision
- The first implementation is `apps/qt/` (decision 0006), for Linux desktop
  and Android from the start.
- The native Android app `apps/compose-android/` (decision 0003) comes
  later, if at all.
- Distribution: APKs and desktop builds are shared directly. No store
  (F-Droid, Play) for now, and F-Droid buildability is not a requirement.
  The web app is hosted publicly but unannounced (decision 0013).

## Consequences
- A Qt Android APK is larger than a Compose one.
- Because of decision 0005, nothing in `docs/spec/` depends on this choice.
