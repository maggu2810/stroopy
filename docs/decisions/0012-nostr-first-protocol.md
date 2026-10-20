# 0012: Live multiplayer starts with Nostr; protocol v1

Status: accepted (2026-10-20)

## Context
The design (`docs/design/multiplayer-and-transports.md`) listed LAN first,
then Nostr. The web app (decision 0010) can only use WebSockets, and playing
Qt against the web app is the first concrete goal.

## Decision
- **Nostr is the first live transport.** LAN and Bluetooth follow later for
  native apps.
- `docs/spec/protocol/v1.md` (`P = 1`) defines rooms, messages and how they
  travel over Nostr: ephemeral events (not stored by relays), one
  throwaway key per room session, a room tag derived from the room code, and
  a default relay set every app uses unless the user changes it.
- Messages are small JSON objects. A periodic `status` message carries each
  player's full state, so a lost event costs at most one heartbeat interval.
- Libraries: libsecp256k1 (MIT, `schnorrsig` module, fetched and built from
  source by CMake) in Qt; @noble/curves in the web app.
- Payloads are **not** encrypted in v1. They hold player names and scores
  only. The spec says so, so users know relays can read them.

## Consequences
- Internet play needs no server of ours, but depends on public relays being
  up; three relays at once give redundancy.
- Event signing must be bit-exact across apps (NIP-01 serialisation,
  BIP-340 Schnorr), so the protocol has conformance vectors like the other
  specs.
