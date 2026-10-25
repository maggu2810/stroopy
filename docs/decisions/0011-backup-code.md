# 0011: One backup code for highscores and settings

Status: accepted (2026-10-20)

## Context
Highscores and settings live on one device. They get lost with a new phone,
a reinstall, or (web app) evicted browser storage. Moving them between our
own implementations (Qt ↔ web ↔ later Compose/SwiftUI) should work too.

Options considered:
1. two formats: highscores as an interop spec, settings app-specific;
2. one format with a highscores section and a settings section.

## Decision
**One** versioned backup format, `docs/spec/backup/v<N>.md` (version `B`,
independent of `R`/`G`/`M`, decision 0007), carried as a text code
(`SYB<B>-…`) that users copy anywhere (notes, mail, messenger).

- The **highscores** section is fully specified.
- The **settings** section is a list of key/length/value items. Keys are
  defined by the spec for the settings every app shares (language,
  colour-blind palette, vibration, player name, relays). Unknown keys are
  skipped, so new shared settings can be added without a new version, and an
  app ignores settings it doesn't have.
- App-specific settings are **not** part of the backup. They don't exist
  yet, and a backup that only one app understands would break the promise
  that any app can restore any backup. If one ever matters, it gets a shared
  key in the spec.

## Consequences
- One "Export backup" / "Import backup" pair per app; users don't have to
  understand two codes.
- The text code reuses game-code machinery (Crockford Base32, CRC-16), and
  every app proves it with conformance vectors.
- A full backup (4 modes × 10 entries + settings) is about 1.3 k characters:
  fine for clipboard and messengers, too long to type.
