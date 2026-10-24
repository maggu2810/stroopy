# Multiplayer without our own server

Status: living document. It explains the approach and the choice of
transports. The binding parts are `docs/spec/share-code/v1.md` and
`docs/spec/protocol/v1.md` (decision 0012); the screens are in
`docs/design/product.md`.

## 1. A game is a seed

Item generation is fully deterministic (`docs/spec/rules/`), so a whole game
is described by a **game descriptor**:

```
rules=1        rules version R (decision 0007), not an app version
mode=classic   classic | chrono | find | tap
seed=<64 bit>  PRNG seed
```

The descriptor holds only what changes the sequence. Per-player display
preferences are **not** part of it: the word *language* (logic is
index-based, so each player reads the words in their own language) and the
colour-blind palette (it changes inks, not logic).

Consequences:

- **Share or replay later** (an asynchronous "challenge") needs **no
  network**. The descriptor, plus an optional result ("I scored 47", so the
  receiver sees the score to beat), is a compact binary record with three
  equivalent forms (share code v1):
  - a **text code** for any messenger, e.g.
    `SYG1-040G-0000-0000-001A-03CT-T`: prefix and version, then Crockford
    Base32 (case-insensitive, no 0/O or 1/I mix-ups) in groups of four, with
    a checksum against typos;
  - a **QR code** with the same text;
  - a **link** `stroopy://play?c=<code>`.

  Apps find a code inside any pasted text, so a whole message or a link
  works as well.
- **Live multiplayer** only exchanges tiny, latency-tolerant messages
  (protocol v1 §2): a periodic `status` with each player's full state (name,
  host flag, supported rules versions, current game, item and score), a
  `start` from the host with the game's share code, and `bye`.

  Each device runs its own timers. The network only *reports*, so delay never
  changes a game. Guests start the 3-2-1-GO countdown when the `start`
  arrives, so starts differ by the network delay; there is no clock
  synchronisation, and fairness across latency is not a goal.

### Fairness and anti-cheat

There is no server, so players trust each other's reports. The game is
casual, so that is acceptable. A possible later addition: a hash of the
answer log (item index → answer, reaction time) at the end of a game, so
peers can recompute the score from the seed.

## 2. Transport options (no Google Play Services)

| transport | range | internet? | infra needed | notes | status |
|---|---|---|---|---|---|
| **Seed sharing (text / QR / link)** | any | no | none | Asynchronous only. | text: done; QR, link: planned |
| **Nostr relays** | worldwide | yes | public relays (not ours) | WebSockets, so it also works in browsers. See §3. | **protocol v1** |
| **LAN / Wi-Fi hotspot** | same network | no | none | mDNS/DNS-SD discovery + TCP. One device hosts; also works over a phone's hotspot. Not available in browsers. | later, native apps |
| **Bluetooth LE (GATT)** | ~10–30 m | no | none | No pairing, tiny messages. Peripheral mode isn't on every device; no Web Bluetooth in Safari. | later, native apps |
| Bluetooth Classic (RFCOMM) | ~10 m | no | none | Pairing is clunky, max ~7 peers. | not planned |
| Wi-Fi Direct (P2P) | ~50 m | no | none | Flaky device support, needs location / nearby-devices permission. | not planned |
| WebRTC data channel | worldwide | yes | signalling + STUN/TURN | True P2P, but heavy (`libwebrtc`) for a few messages per second. | not planned |
| MQTT public broker | worldwide | yes | public broker | Public test brokers have no SLA. | not planned |
| Matrix / XMPP | worldwide | yes | account on a homeserver | Needs an account: too heavy for a casual game. | not planned |
| libp2p / Tor / I2P | worldwide | yes | bootstrap/relay nodes | Overkill. | not planned |
| Google Nearby Connections | — | — | — | Needs Play Services. | **excluded** |

## 3. Internet play without a server: Nostr

Nostr is an open protocol: clients exchange signed JSON events over
WebSockets with **public relays**, and hundreds of independent ones are
running. We run **no** server ourselves. As specified in protocol v1 §3:

- Every message is the content of an **ephemeral event** (kind 25713):
  relays forward it live and don't store it.
- A **room code** (`SYR1-` + 8 Crockford Base32 characters) gives the room
  tag that all players subscribe to.
- Each player creates a **throwaway key** when entering a room and discards
  it when leaving.
- Every app uses the same **three default relays** and publishes to all of
  them; received events are deduplicated by id. The list is configurable.
- Messages are signed but **not encrypted** in v1; they carry only names and
  scores, and the apps say so.

Risks: public relays may rate-limit, drop events or go offline. Three relays
at once and the periodic full `status` make that tolerable. Without any
reachable relay there is no internet play; sharing codes still works.

## 4. Transport abstraction

Game logic never depends on a concrete transport (AGENTS.md, rule 4). Each
implementation separates:

- a **session**: join a room, send a message to everyone, receive messages
  with their sender, leave;
- the **room logic** of protocol v1 §4 (presence, host, starting, ranking),
  which works on messages only.

A LAN or Bluetooth transport later provides another session with the same
messages, in a new protocol version that adds its mapping.

Order:
1. Single player, deterministic seed, share and replay a sequence.
2. **Nostr** live multiplayer: the only transport a browser can use, so it
   comes first (decision 0012).
3. **LAN** for native apps.
4. **Bluetooth LE** for offline play without shared Wi-Fi.

## 5. Ideas for later

- A Chrono variant where a 10-streak takes 1 s off the opponents (as in the
  original). It breaks pure "same sequence" comparability, so it would be
  its own mode in a new rules version.
- The answer-log hash from §1.
