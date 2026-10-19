# 0003: Android tech stack

Status: accepted (2026-10-19)

## Decision
- **Kotlin** and **Jetpack Compose** (AndroidX), Gradle with Kotlin DSL and a
  version catalog.
- Game logic, share codes and the protocol live in a **pure-Kotlin module**
  (no Android imports) that is unit-tested on the JVM against
  `docs/spec/test-vectors/`.
- No Google Play Services, Firebase or proprietary SDKs (see decision 0002 and
  AGENTS.md).

## Consequences
The pure-Kotlin core could later become a Kotlin Multiplatform module if we
ever want to share it. Decision 0005 does not require that, but keeps it
possible.
