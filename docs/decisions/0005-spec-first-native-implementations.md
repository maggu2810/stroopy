# 0005: Spec-first, independent native implementations

Status: accepted (2026-10-19)

## Context
Beyond Android we want iOS and desktop (Linux, Windows, macOS) at some point.
Options:
- (a) one cross-platform UI framework (Flutter, Compose Multiplatform,
  React Native, Qt, …);
- (b) a shared core (e.g. Kotlin Multiplatform or Rust) with native UIs;
- (c) fully independent native implementations driven by precise specs.

The game core is small (a PRNG, item generation, timing, scoring, encoding,
protocol). Most of the code is UI and platform integration (BLE, NSD,
WebSocket, QR camera, haptics), which is platform-specific in every option.
With AI-assisted development, writing a small core several times is cheap.
Keeping several implementations **compatible** is the hard part.

## Decision
Option **(c)**: independent, idiomatic implementations (one per toolkit,
in `apps/<stack>[-<platform>]/`) built from `docs/spec/`. A platform may have
more than one implementation. Compatibility is enforced by **language-neutral conformance
test vectors** (`docs/spec/test-vectors/*.json`). Every implementation's test
suite must load and pass them.

Rules:
1. Specs are normative. If an implementation and the spec disagree, the spec
   wins. Ambiguities are fixed in the spec first, then in all apps.
2. Every behaviour that affects interoperability (sequence, timing, scoring,
   share code, protocol) has test vectors.
3. A protocol or rules change bumps a version number. Implementations state
   which versions they support.

## Consequences
- + Best native look, feel and platform integration. No framework lock-in.
- + Independent implementations act as a cross-check that the spec is
  unambiguous.
- − Features and bug fixes have to be done N times. UI/UX docs
  (`docs/design/`) should be detailed enough to keep apps consistent.
- **Desktop:** "native" would mean three toolkits (GTK/AppKit/WinUI). A
  single cross-platform toolkit is allowed instead (see decision 0006: Qt).
- Escape hatch: if duplicated core logic becomes a burden, the pure-Kotlin
  core (decision 0003) can be turned into a Kotlin Multiplatform library for
  iOS and desktop without changing the specs.
