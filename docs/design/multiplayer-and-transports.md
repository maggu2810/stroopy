# Multiplayer without our own server

Status: **proposal**, partly decided. Live multiplayer over Nostr is now
specified in `docs/spec/protocol/v1.md` (decision 0012); where this document
differs (message names, clock sync, order of transports), the spec wins.
Decisions go to `docs/decisions/` once they are agreed.

## 1. Key insight: a game is a seed

If item generation is fully deterministic (see `docs/spec/rules/`), a whole
game is described by a **game descriptor**:

```
rules=1        rules version R (decision 0007), not an app version
mode=classic   classic | chrono | find | tap
seed=<64 bit>  PRNG seed
```

The descriptor holds only what changes the sequence. Per-player display
preferences are **not** part of it: the word *language* (logic is
index-based, so each player reads the words in their own language) and the
colour-blind palette (it changes inks, not logic). The share-code format
version `S` is carried by the encoding itself (prefix `SY<S>-`).

Consequences:

- **Share or replay later** (asynchronous "challenge") needs **no network**.
  The descriptor (plus an optional result: "I reached 47", so the receiver
  sees the score to beat) is serialised into a compact binary record and
  offered in three equivalent forms:
  - a **QR code**, shown on the result screen and scanned in the app;
  - a **text code** that can be sent by any messenger and pasted into the
    app, e.g. `SY1-K7Q2-9F3A-XW1P-M4`: a version prefix, then Crockford
    Base32 (case-insensitive, no 0/O or 1/I mix-ups), grouped for
    readability, with a checksum to catch typos;
  - a **link**, e.g. `stroopy://play?c=K7Q29F3AXW1PM4`, which carries the
    same code (opens the app directly if installed).

  The app accepts a pasted code from the clipboard and also accepts the
  link form pasted as text.
- **Live multiplayer** only has to exchange tiny, latency-tolerant messages:

| message | content |
|---|---|
| `hello` | player name, supported protocol and rules versions; app name and version (informational only) |
| `game` | descriptor plus a start time (host clock + countdown) |
| `progress` | player, item index reached, score, alive/dead, timestamp |
| `finished` | player, final score, item index where they died, total time |

  Each device runs its own timer locally. The network only *reports*, so
  packet delay never affects fairness. A few hundred ms of latency is fine,
  and roughly 1–5 messages per second per player is enough.

### Fairness and anti-cheat

There is no server, so peers have to trust each other's reports. The game is
casual, so trust is acceptable. Optional later step: send a hash of the answer
log (`item index → answer, reaction ms`) at the end, so peers can recompute and
check the score from the seed.

### Clock sync

A start time based on the host clock plus a simple NTP-style offset estimate
over the chosen transport (send `ping t0`, receive `pong t0 t1`) is enough to
start everyone's countdown within about 50 ms.

## 2. Transport options (no Google Play Services)

| transport | range | internet? | infra needed | Android APIs | notes |
|---|---|---|---|---|---|
| **Seed sharing (link / QR / text)** | any | no | none | Intents, a QR lib | Asynchronous only. Simplest, and it ships first. |
| **LAN / Wi-Fi hotspot** | same network | no | none | `NsdManager` (mDNS/DNS-SD) + TCP sockets | One phone hosts, the others discover it. Also works when one phone opens a hotspot. Very reliable, simple. |
| **Wi-Fi Direct (P2P)** | ~50 m | no | none | `WifiP2pManager` | Needs no access point, but device support is flaky, and it needs location or `NEARBY_WIFI_DEVICES` permission. |
| **Bluetooth Classic (RFCOMM)** | ~10 m | no | none | `BluetoothServerSocket` | Pairing or discovery is clunky. Max ~7 peers. Needs `BLUETOOTH_CONNECT/SCAN`. |
| **Bluetooth LE (GATT)** | ~10–30 m | no | none | `BluetoothLeAdvertiser` + GATT server | Discovery needs no pairing and messages are tiny, so it fits well. Peripheral mode isn't on every device. |
| **Nostr relays** | worldwide | yes | public relays (not ours) | WebSocket (OkHttp/Ktor) | See §3. Recommended for internet play. |
| **WebRTC data channel** | worldwide | yes | signalling + STUN (TURN for ~10–20 % of NATs) | `libwebrtc` (~10 MB) | Peer-to-peer, but still needs signalling, and that could go over Nostr. Heavy for our tiny traffic. |
| **MQTT public broker** | worldwide | yes | public broker | Paho/HiveMQ client | Public test brokers have no SLA and must not be relied on. |
| **Matrix / XMPP** | worldwide | yes | account on a homeserver | SDKs | Needs an account, so too heavy for a casual game. |
| **libp2p / Tor / I2P** | worldwide | yes | public bootstrap/relay nodes | big libraries | Overkill. |
| Google Nearby Connections | — | — | — | — | **Excluded**: needs Play Services. |

## 3. Internet play without a server: Nostr relays (recommended)

Nostr is an open protocol. Clients exchange signed JSON events over
WebSockets with **public relays**, and hundreds of independent ones are
running. We would run **no** server ourselves:

- Each install generates a throwaway keypair (secp256k1/Schnorr, e.g. via
  `secp256k1-kmp` or `bouncycastle`).
- **Ephemeral events** (kind 20000–29999) are relayed live and *not stored*,
  which suits game traffic.
- The **room code** (e.g. 6 characters, shown as text or QR code) is hashed
  into a tag. All players subscribe to that tag on the same 2–3 relays, for
  redundancy.
- Optional: encrypt payloads with a key derived from the room code, so relays
  and other people can't read them.
- The relay list is user-configurable. Anyone (including us) *may* host a
  relay, but nobody has to.

Risks: public relays may rate-limit or drop events and may go offline. We
mitigate that by using several relays at once and by deduplicating by event
id. Also: no relay, no internet play. Seed sharing and local play still work.

**Alternative or complement:** WebRTC data channels, with Nostr used only for
signalling. That gives true P2P, but it adds `libwebrtc` and needs STUN/TURN.
Not worth it for ~5 msg/s. Revisit only if relays prove unreliable.

## 4. Proposed transport abstraction

```
interface Transport {
    suspend fun host(room: RoomInfo): Session      // LAN/BT: listen; Nostr: create room code
    suspend fun join(target: JoinTarget): Session  // discovered peer or room code
}
interface Session {
    val incoming: Flow<Message>
    suspend fun broadcast(msg: Message)
    val peers: StateFlow<List<Peer>>
    suspend fun close()
}
```

Game logic only sees `Session`, and messages are versioned. Suggested
implementation order:

1. Single player, deterministic seed, share/replay a sequence (no network).
2. **Nostr** internet multiplayer: the only transport a browser can use
   (decision 0012), so it comes first.
3. **LAN** (NSD + TCP) for native apps.
4. **Bluetooth LE** for offline play without shared Wi-Fi.

## 5. Multiplayer UX (simultaneous, identical sequence)

- The lobby shows a room code or QR code, the players (2–8), the mode, and a
  ready state.
- The host presses start, every device receives `game`, and a synchronised
  3-2-1-GO countdown follows.
- During play, a compact strip shows each opponent: name, current item index
  and score, and ✗ with "died at #31" once they fail.
- The result screen shows a ranking (score, then time to die/finish as
  tie-break), and the seed can be shared for a rematch or a later challenge.
- Optional twist from the original: a 10-streak takes 1 s off opponents in
  Chrono mode. This is a mode flag (`attack=1`), because it breaks pure
  "same sequence" comparability.
