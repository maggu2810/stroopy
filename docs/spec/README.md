# docs/spec/: normative specifications

Everything an implementation needs to be **compatible** with every other one.
Written in language-neutral terms (pseudo code, byte layouts). Versioning
follows [decision 0007](../decisions/0007-compatibility-versions.md):

| area | file | version | governs |
|---|---|---|---|
| rules | `rules/v<N>.md` | `R` | PRNG, item generation, timing, scoring per mode |
| share code | `share-code/v<N>.md` | `S` | game descriptor and result encodings (binary, text, QR, link) |
| protocol | `protocol/v<N>.md` | `P` | multiplayer rooms and messages, and how they travel (Nostr) |
| backup | `backup/v<N>.md` | `B` | highscores and shared settings as a backup code |

Conformance vectors: `test-vectors/<area>/v<N>/*.json`. Each file holds
inputs and expected outputs, including negative cases. An implementation that
claims version `N` of an area must pass all of that area's `vN` vectors.

A published version is **frozen**: clarifications only, never behaviour
changes. Changes go into `v<N+1>`.

`docs/design/` holds proposals, UX and rationale. Once something is agreed,
the binding part moves here.
