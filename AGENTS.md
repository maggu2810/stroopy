# AGENTS.md: Stroopy (FOSS remake of "True Color")

Guidance for humans and AI agents working in this repository.

## Goal

**Stroopy** (`de.maggu2810.android.stroopy`, GPL-3.0-or-later) is a free
and open-source game for Linux desktop, Android (including Android TV) and iOS
(Windows and macOS later), in the spirit of the discontinued "True Color"
(`com.aurelhubert.truecolor`, last known version 3.0.2):

- A Stroop-effect colour game: is the word's meaning the same as its ink
  colour? The game gets faster as you progress.
- **No Google dependencies**: no Play Services, Firebase, Play Billing,
  Play Games or ads/analytics SDKs. It must run on de-Googled phones
  (LineageOS, GrapheneOS, /e/OS…) and stay eligible for **F-Droid**, even
  though publishing there is not planned for now (decision 0008).
  (AndroidX/Jetpack libraries are fine, since they are open source and need
  no Play Services.)
- **Single player** and **simultaneous multiplayer** on an *identical* colour
  sequence, with live progress of the other players (score, item reached,
  died).
- **Replayable sequences**: after a game, its sequence (seed + mode + rules
  version) can be shared as a game code and played later by someone else.
- Multiplayer without our own server: public Nostr relays first, LAN and
  Bluetooth later.
  See `docs/design/multiplayer-and-transports.md`.

## Mental model

An app is an implementation of specifications. It can be built for any
platform with any toolkit:

1. **Interop specs** (`docs/spec/`): binding, versioned, frozen once
   published, and checked by conformance vectors. Every app must follow them
   exactly so that shared codes and multiplayer work across apps:
   - **rules `R`**: how a game descriptor is played (PRNG, sequence, speed
     curve, scoring);
   - **game `G`**: how a game (descriptor and optional result) is encoded as
     a game code (bytes, text, QR, link);
   - **multiplayer `M`**: room invitations, and how apps talk during live
     multiplayer;
   - **backup `B`**: how highscores and settings are encoded as a backup
     code.
2. **Product spec** (`docs/design/`): modes, screens, flows and
   accessibility (`product.md`), and one shared look (`visual-style.md`).
   Apps follow it for a consistent experience but adapt it to their
   platform's idioms. It does not affect interoperability.

An implementation declares which `R`/`G`/`M`/`B` versions it supports and proves
it by passing their vectors. App versions are irrelevant for compatibility.

## Repository layout

```
AGENTS.md                 this file
apps/                     our implementations, one dir per toolkit/language
  qt/                     C++/Qt 6/QML → Linux desktop, Android (active)
  web/                    TypeScript/Bun/Preact PWA → browsers, iOS (active)
  compose-android/        Kotlin + Jetpack Compose → Android (postponed)
docs/
  spec/                   NORMATIVE, versioned specs (frozen once published)
    rules/v<N>.md         item generation, timing, scoring
    game/v<N>.md          game code: descriptor encoding (binary, text, QR, link)
    multiplayer/v<N>.md   invitations, messages, Nostr transport
    backup/v<N>.md        highscores + settings backup code
    test-vectors/<area>/v<N>/   JSON conformance vectors
  design/                 proposals, UX, rationale, default Nostr relays
  research/               analyses of other apps, protocols, libraries
  decisions/              decision records (NNNN-title.md)
third-party/              material from other apps, for STUDY ONLY
  README.md
  <package-id>/
    NOTES.md              provenance (source, version, sha256)
    apk/                  git-ignored
    decompiled/           git-ignored (jadx + apktool output)
.github/workflows/        CI: web-pages.yml publishes apps/web to GitHub Pages
tools/                    helper scripts (e.g. fetch-original.sh)
  reference/              Python reference implementations + vector generators
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
   versions playable for shared seeds. Published spec versions are frozen.
   Cover them with conformance vectors (decision 0007).
4. **Network only reports, never drives.** Timers run locally. Multiplayer
   messages carry progress and results. Game logic depends on a `Transport` /
   `Session` abstraction, not on a concrete transport.
5. **Docs first for decisions.** Record non-trivial choices (stack, licence,
   protocol, PRNG) in `docs/decisions/`.
6. **Spec-first, independent implementations.** Every app is built from
   `docs/spec/` and must pass `docs/spec/test-vectors/`. Fix ambiguities in
   the spec first, then in every app. Never let an app become the de-facto
   spec (see decision 0005).
7. **Accessibility.** Keep a colour-blind palette, a vibration toggle and
   localised colour words.

## Decided

- Name, ids, link scheme and code prefixes: **Stroopy**,
  `de.maggu2810.android.stroopy`, `stroopy://`, **`SYG`** game, **`SYM`**
  multiplayer invitation, **`SYB`** backup
  (`docs/decisions/0001-app-name-and-id.md`)
- Licence: **GPL-3.0-or-later** (`LICENSE`, `docs/decisions/0002-license.md`).
  Every source file starts with `SPDX-License-Identifier: GPL-3.0-or-later`.
- Native Android app: **Kotlin + Jetpack Compose**, game core as a
  pure-Kotlin module, **postponed**
  (`docs/decisions/0003-android-tech-stack.md`)
- PRNG: **SplitMix64** (`docs/decisions/0004-prng-splitmix64.md`)
- Platform strategy: spec-first, independent implementations held together
  by conformance vectors
  (`docs/decisions/0005-spec-first-native-implementations.md`)
- Desktop and Android: **Qt 6 + QML/C++** in `apps/qt/`
  (`docs/decisions/0006-desktop-qt.md`)
- Compatibility: independent integer versions for **rules**, **game**,
  **multiplayer** and **backup**. App versions never decide compatibility
  (`docs/decisions/0007-compatibility-versions.md`)
- First implementation: **`apps/qt/`** for Linux desktop + Android; builds
  are shared directly, no store (`docs/decisions/0008-qt-first.md`)
- Toolchains: always the **latest** stable releases and their newest
  features (`docs/decisions/0009-latest-toolchains.md`)
- iOS and browsers: a **PWA** in `apps/web/` (TypeScript, Bun, Preact)
  (`docs/decisions/0010-web-app-pwa.md`)
- Backup: one **backup code** for highscores and shared settings
  (`docs/decisions/0011-backup-code.md`)
- Live multiplayer: **Nostr first**, multiplayer v1; players join with an
  invitation that carries the room's relays
  (`docs/decisions/0012-nostr-first-multiplayer.md`)
- Web hosting: **GitHub Pages**, built by GitHub Actions
  (`docs/decisions/0013-web-hosting-github-pages.md`)
- App versions: every change to an app bumps its **SemVer** version
  (`docs/decisions/0014-app-version-bump-on-every-change.md`)

## Open decisions

- Kirigami on top of Qt Quick or plain Qt Quick (plain Qt Quick so far).

## Conventions

- Commit messages: imperative mood, short subject line.
- Agents commit their work themselves, in logical groups (one concern per
  commit), without waiting to be asked. They don't push unless asked.
- Keep `docs/` up to date with behaviour changes in the same commit.
- Every commit that changes what an app builds or ships bumps that app's
  version in the same commit (`apps/qt/CMakeLists.txt`,
  `apps/web/package.json`): `PATCH` for fixes, `MINOR` for features or
  changed behaviour. Tests, READMEs, CI and `docs/` alone don't bump
  (decision 0014).
- Decision records are append-only: revise by a new record and link it from
  the old one's status (`docs/decisions/README.md`).
