# AGENTS.md: Stroopy (FOSS remake of "True Color")

Guidance for humans and AI agents working in this repository.

## Goal

**Stroopy** (`de.maggu2810.android.stroopy`, GPL-3.0-or-later) is a free
and open-source game, Android first with iOS and desktop later, in the spirit of the discontinued "True Color"
(`com.aurelhubert.truecolor`, last known version 3.0.2):

- A Stroop-effect colour game: is the word's meaning the same as its ink
  colour? The game gets faster as you progress.
- **No Google dependencies**: no Play Services, Firebase, Play Billing,
  Play Games or ads/analytics SDKs. It must run on de-Googled phones
  (LineageOS, GrapheneOS, /e/OS…) and be eligible for **F-Droid**.
  (AndroidX/Jetpack libraries are fine, since they are open source and need
  no Play Services.)
- **Single player** and **simultaneous multiplayer** on an *identical* colour
  sequence, with live progress of the other players (score, item reached,
  died).
- **Replayable sequences**: after a game, its sequence (seed + mode + rules
  version) can be shared and played later by someone else.
- Multiplayer without our own server: LAN, Bluetooth and public Nostr relays.
  See `docs/design/multiplayer-and-transports.md`.

## Repository layout

```
AGENTS.md                 this file
apps/                     our implementations, one native app per platform
  android/                Kotlin + Jetpack Compose
  (ios/, desktop/)        future
docs/
  spec/                   NORMATIVE specs: rules, share code, protocol
    test-vectors/         language-neutral JSON conformance vectors
  design/                 proposals, UX, rationale
  research/               analyses of other apps, protocols, libraries
  decisions/              decision records (NNNN-title.md)
third-party/              material from other apps, for STUDY ONLY
  README.md
  <package-id>/
    NOTES.md              provenance (source, version, sha256)
    apk/                  git-ignored
    decompiled/           git-ignored (jadx + apktool output)
tools/                    helper scripts (e.g. fetch-original.sh)
```

## Ground rules

1. **Clean-room separation.** `third-party/` is for understanding behaviour
   only. Never copy code, layouts, drawables, sounds, fonts or string
   resources from it into `apps/`. Write findings in our own words in
   `docs/research/`, and implement from `docs/spec/` and `docs/design/`. Third-party binaries
   and decompiled output are never committed (see `.gitignore`). Regenerate
   them with `tools/fetch-original.sh`.
2. **No proprietary dependencies.** Before adding a library, check its licence
   (it must be OSI-approved and GPL-compatible) and that it pulls in nothing
   from `com.google.android.gms`, `com.google.firebase` or similar. Use no
   tracking and no network calls except the multiplayer transports the user
   opts into.
3. **Determinism is a contract.** Item generation and timing must be pure
   functions of `(rulesVersion, mode, seed, item index / player answers)`. Use
   our own specified PRNG (not `java.util.Random` or `Math.random`), so every
   platform and version produces identical sequences. Any change to
   generation or timing **bumps the rules version** and keeps the old
   versions playable for shared seeds. Cover this with golden tests.
4. **Network only reports, never drives.** Timers run locally. Multiplayer
   messages carry progress and results. Game logic depends on a `Transport` /
   `Session` abstraction, not on a concrete transport.
5. **Docs first for decisions.** Record non-trivial choices (stack, licence,
   protocol, PRNG) in `docs/decisions/`.
6. **Spec-first, native per platform.** Every app is built from
   `docs/spec/` and must pass `docs/spec/test-vectors/`. Fix ambiguities in
   the spec first, then in every app. Never let an app become the de-facto
   spec (see decision 0005).
7. **Accessibility.** Keep a colour-blind palette, a vibration toggle and
   localised colour words.

## Decided

- Name and id: **Stroopy**, `de.maggu2810.android.stroopy`
  (`docs/decisions/0001-app-name-and-id.md`)
- Licence: **GPL-3.0-or-later** (`LICENSE`, `docs/decisions/0002-license.md`).
  Every source file starts with `SPDX-License-Identifier: GPL-3.0-or-later`.
- Platform strategy: spec-first, one native app per platform, held together
  by conformance vectors (`docs/decisions/0005-spec-first-native-implementations.md`)
- Android: **Kotlin + Jetpack Compose**, game core as a pure-Kotlin module
  (`docs/decisions/0003-android-tech-stack.md`)
- PRNG: **SplitMix64** (`docs/decisions/0004-prng-splitmix64.md`)

## Open decisions

- Desktop toolkit (when we get there; see decision 0005).

## Conventions

- Commit messages: imperative mood, short subject line.
- Keep `docs/` up to date with behaviour changes in the same commit.
