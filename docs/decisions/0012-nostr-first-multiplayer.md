# 0012: Live multiplayer starts with Nostr; multiplayer v1

Status: accepted (2026-10-20)

## Context
Live multiplayer needs a transport without a server of our own
(`docs/design/multiplayer-and-transports.md` compares the options). The web
app (decision 0010) can only use WebSockets, and playing the Qt app against
the web app is the first concrete goal.

Players can only meet over Nostr if they use the same relays. Each player
may configure their own relays, and the default relays of an app change
over time as public relays go offline. A room id alone therefore doesn't
say where the room is. LAN and Bluetooth, planned for later, find rooms
nearby by themselves and need different information again.

## Decision
- **Nostr is the first live transport.** LAN and Bluetooth may follow later
  for native apps, carrying the same messages.
- `docs/spec/multiplayer/v1.md` (`M = 1`) defines rooms, invitations,
  messages and how they travel over Nostr: ephemeral events (not stored by
  relays), one throwaway key per room session, and a room tag derived from
  the room id.
- **Players join with an invitation, not with a room code.** An invitation
  (`SYM<M>-…`, Crockford Base32 with CRC-16 like the game and backup codes)
  holds the room id and a list of tagged transport entries: for Nostr,
  1–8 relay URLs. Readers skip unknown tags, so a new transport can be added
  without breaking older apps. It is shared as text, link or QR code. There
  is no short form to type, since a room id alone can't be joined.
- **A room's relays are fixed by its creator.** They are the creator's
  relay setting when the room is created, and every player uses exactly
  these relays, whatever their own setting is.
- **Default relays are not part of the spec.** Apps ship a default list
  taken from `docs/design/nostr-relays.md`, which records recommended
  lists with the date they were chosen. Changing the list needs no new
  multiplayer version, because the invitation carries the relays.
- Messages are small JSON objects. A periodic `status` message carries each
  player's full state, so a lost event costs at most one heartbeat interval.
- Libraries: libsecp256k1 (MIT, `schnorrsig` module, fetched and built from
  source by CMake) in Qt; @noble/curves in the web app.
- Payloads are **not** encrypted in v1. They hold player names and scores
  only. The spec says so, so users know relays can read them.

## Consequences
- Internet play needs no server of ours, but depends on public relays being
  up; several relays at once give redundancy.
- Players always meet on the same relays, regardless of their settings or
  app versions. An invitation is long (about 140 characters with three
  relays), which is fine for QR codes, messengers and links but not for
  typing or reading aloud.
- Event signing must be bit-exact across apps (NIP-01 serialisation,
  BIP-340 Schnorr), so multiplayer has conformance vectors like the other
  specs.
