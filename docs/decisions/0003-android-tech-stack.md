# 0003: Native Android app: Kotlin and Jetpack Compose

Status: postponed (2026-10-19)

## Context
Android is covered first by the Qt app (decision 0008). A native Android
implementation may follow later, for the best platform integration and a
smaller APK.

## Decision
When `apps/compose-android/` starts, it uses:
- **Kotlin** and **Jetpack Compose** (AndroidX), Gradle with Kotlin DSL and a
  version catalog;
- game logic, share codes, backup and protocol in a **pure-Kotlin module**
  (no Android imports), unit-tested on the JVM against
  `docs/spec/test-vectors/`;
- no Google Play Services, Firebase or other proprietary SDKs (AGENTS.md,
  decision 0002).

## Consequences
The app is postponed, not dropped. It is an independent implementation of
the specs (decision 0005) and shares no code with the Qt or web app.
