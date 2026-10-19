# AGENTS.md: True Colors (FOSS remake)

Guidance for humans and AI agents working in this repository.

## Goal

A free and open-source Android game in the spirit of the discontinued
"True Color" (`com.aurelhubert.truecolor`, last known version 3.0.2):

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
app/                      our own app source (Android project lives here)
docs/
  research/               analyses of other apps, protocols, libraries
  design/                 specs of our game: rules, protocol, UI
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
   resources from it into `app/`. Write findings in our own words in
   `docs/research/`, and implement from `docs/design/`. Third-party binaries
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
6. **Accessibility.** Keep a colour-blind palette, a vibration toggle and
   localised colour words.

## Open decisions

- Licence for our code (GPL-3.0-or-later is suggested; it fits F-Droid).
- Tech stack: proposed **Kotlin + Jetpack Compose**, with game logic in a
  pure-Kotlin module so it can be tested on the JVM (and later shared via
  Kotlin Multiplatform if desired).
- App name and id: avoid confusion with the original name and trademark.
  The underlying concept is the **Stroop effect** (J. R. Stroop, 1935), which
  is a natural source for a name. Example id: `de.maggu2810.<name>`.
- PRNG choice: e.g. SplitMix64 or xoshiro256**. Specify it in
  `docs/design/game-rules.md`.

## Conventions

- Commit messages: imperative mood, short subject line.
- Keep `docs/` up to date with behaviour changes in the same commit.
