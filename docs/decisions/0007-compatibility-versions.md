# 0007: Compatibility versions, independent of app versions

Status: accepted (2026-10-19)

## Context
Shared codes, backups and multiplayer must work between different
implementations and different app releases. Two apps with the same
version number (both start at `0.1.0`) need not be compatible, and two
with different numbers may well be.

## Decision
Interoperability is governed by **independent, monotonically increasing
integer versions**, each with its own spec and conformance vectors:

| version | governs | carried in | bumped when |
|---|---|---|---|
| **rules** (`R`) | PRNG use, item generation, timing curve, scoring per mode | the game descriptor | any change that alters a sequence, timing or score for the same seed |
| **share code** (`S`) | binary layout of the descriptor and result, text/QR/link encodings | the code itself (text prefix `SYG<S>-`, first byte of the binary) | the encoding changes incompatibly |
| **protocol** (`P`) | multiplayer rooms, messages and their semantics, and their transport mapping | the room code (`SYR<P>-`), the room tag and every message (`"p"`) | message format or meaning changes incompatibly |
| **backup** (`B`) | highscores and shared settings as a backup code (decision 0011) | the code itself (text prefix `SYB<B>-`, first byte of the binary) | the encoding changes incompatibly |

App versions are **informational only** (shown in the UI). They are never
used for compatibility decisions.

### Rules
1. Versions are integers from 1 to 255. There are no minor versions.
   Backward-compatible additions are only allowed where the spec explicitly
   defines extension points (e.g. settings keys that old readers skip).
2. Specs are versioned files: `docs/spec/<area>/v<N>.md` with the areas
   `rules`, `share-code`, `protocol` and `backup`. A spec starts as
   **draft** and is **frozen** once implementations pass its vectors and the
   version is in use. A frozen version allows clarifications only, never
   behaviour changes.
3. Conformance vectors are per version:
   `docs/spec/test-vectors/<area>/v<N>/*.json`.
4. Each implementation declares what it supports in its README (and in its
   about screen), e.g. `rules 1–2, share code 1, protocol 1, backup 1`. Its
   test suite runs **all** vectors of every claimed version. Claiming a
   version without passing its vectors is not allowed.
5. A share code is playable if the app supports its `S` **and** the `R`
   inside it. Otherwise the app shows "this code needs a newer version"
   instead of failing silently. Negative vectors (unknown versions, bad
   checksums) are part of the conformance suite.
6. Multiplayer: the room code fixes `P`. Peers list their supported `R` in
   every `status` message, and the host only offers rules versions that
   every peer supports (protocol v1 §4).
7. Apps should keep supporting old rules versions, so old shared codes stay
   playable for as long as is reasonable.

## Consequences
- Any two apps can be checked for compatibility just by comparing their
  declared version sets.
- A new implementation can be developed version by version against the
  frozen vectors.
- Each spec area evolves at its own pace. A new share-code encoding does not
  force a rules bump, and the other way round.
