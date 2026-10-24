# 0001: App name, application ids and code prefixes

Status: accepted (2026-10-19)

## Context
We remake the discontinued "True Color" (`com.aurelhubert.truecolor`). Reusing
its name could confuse users and may conflict with trademarks. The game
concept is the Stroop effect (J. R. Stroop, 1935).

Users exchange three kinds of codes: games (with an optional result), rooms
and backups. They should be recognisable as ours and tell their kind at a
glance.

## Decision
- App name: **Stroopy**
- Base application id / package: **`de.maggu2810.android.stroopy`**. Each
  implementation uses its own Android id, so several can be installed side
  by side: the Compose app uses the base id, the Qt app
  `de.maggu2810.android.stroopy.qt`.
- Link scheme: **`stroopy://`** (`stroopy://play?c=…`, `stroopy://join?r=…`).
- Text code prefixes: `SY` for Stroopy, a letter for the kind, then the
  version of the spec that defines it (decision 0007):

  | prefix | kind | example |
  |---|---|---|
  | `SYG<S>-` | game (share code) | `SYG1-040G-0000-0000-001A-03CT-T` |
  | `SYR<P>-` | room | `SYR1-7K3M-Q9TD` |
  | `SYB<B>-` | backup | `SYB1-…` |

## Consequences
The name refers to the well-known psychology concept, not to the original app.
A web search (2026-10-19) found no existing Android app called "Stroopy".
Parsers look for their own prefix only, so a code of one kind is never
mistaken for another.
