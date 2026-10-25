# 0001: App name, application ids and code prefixes

Status: accepted (2026-10-19)

## Context
We remake the discontinued "True Color" (`com.aurelhubert.truecolor`). Reusing
its name could confuse users and may conflict with trademarks. The game
concept is the Stroop effect (J. R. Stroop, 1935).

Users exchange three kinds of codes: games (with an optional result),
invitations to a multiplayer room, and backups. They should be recognisable
as ours and tell their kind at a glance.

## Decision
- App name: **Stroopy**
- Base application id / package: **`de.maggu2810.android.stroopy`**. Each
  implementation uses its own Android id, so several can be installed side
  by side: the Compose app uses the base id, the Qt app
  `de.maggu2810.android.stroopy.qt`.
- Link scheme: **`stroopy://`** (`stroopy://play?c=…`, `stroopy://join?c=…`).
- Text code prefixes: `SY` for Stroopy, then the letter of the spec that
  defines the code, then that spec's version (decision 0007). The letter
  names the kind of code and its spec at once:

  | prefix | kind | spec | example |
  |---|---|---|---|
  | `SYG<G>-` | game (descriptor and optional result) | game | `SYG1-040G-0000-0000-001A-03CT-T` |
  | `SYM<M>-` | multiplayer room invitation | multiplayer | `SYM1-796K-QTHW-040H-…` |
  | `SYB<B>-` | backup | backup | `SYB1-…` |

  The rules (`R`) have no code of their own. A game code carries its rules
  version.

## Consequences
The name refers to the well-known psychology concept, not to the original app.
A web search (2026-10-19) found no existing Android app called "Stroopy".
Parsers look for their own prefix only, so a code of one kind is never
mistaken for another.
