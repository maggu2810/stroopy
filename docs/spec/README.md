# docs/spec/: normative specifications

Everything an implementation needs to be **compatible** with every other one:
same sequences from the same seed, same share codes, same multiplayer
protocol. Written in language-neutral terms (pseudo code, byte layouts).

- `game-rules.md`: PRNG, item generation, timing, scoring per mode (rules v1)
- `share-code.md`: binary game descriptor and its text/QR/link encodings
- `protocol.md`: multiplayer messages, independent of transport
- `test-vectors/`: machine-readable JSON vectors (inputs → expected outputs)
  that every implementation runs in its test suite

`docs/design/` holds proposals, UX and rationale. Once something is agreed,
the binding part moves here.
