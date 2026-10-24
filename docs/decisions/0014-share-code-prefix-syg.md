# 0014: Share codes get the prefix `SYG`

Status: accepted (2026-10-24)

## Context
The text codes used `SY<S>-` (game), `SYR<P>-` (room) and `SYB<B>-`
(backup). Only the game code had no type letter, so the three forms were
inconsistent to read and to explain.

Share code v1 is already implemented and shared within the maintainer's
family only: the web app is public but unannounced, and the Android app is
not distributed. Changing v1 now costs nothing for real users.

## Decision
- Share codes use **`SYG<S>-`** (`G` for game), e.g.
  `SYG1-040G-0000-0000-001A-03CT-T`. Room codes stay `SYR<P>-`, backup
  codes `SYB<B>-`.
- Share code v1 is changed **in place**, as an exception to "published
  versions are frozen" (decision 0007): no version bump, no support for the
  old `SY1-` form. A conformance vector checks that `SY1-…` is rejected.
- The binary record is unchanged. Only the text prefix changes, so QR codes
  and links change with it.

## Consequences
- Codes shared before 2026-10-24 (`SY1-…`) no longer work. Stored
  highscores and backups are not affected: they don't contain the text
  prefix.
- Protocol v1 vectors were re-signed, because their `start` and `status`
  messages carry share codes.
- After this change, frozen means frozen: a further change to a published
  code needs a new version.
