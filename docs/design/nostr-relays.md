# Default Nostr relays

Status: living document. Every app ships a default relay list, used to
create a room when the player hasn't set relays of their own (multiplayer
v1 §3.3). The list is not part of the spec: an invitation always carries
its room's relays, so apps with different defaults still play together
([decision 0012](../decisions/0012-nostr-first-multiplayer.md)).

How to use this file:

- A new implementation, or an app updating its defaults, takes the
  **newest** list.
- New lists go **on top**. Old lists stay below as history.
- A new list is needed when a relay is gone for good or keeps failing (rate
  limits, dropped ephemeral events). Note the reason.
- Candidates must accept ephemeral events (kind 25713) from fresh keys
  without payment or authentication (NIP-42), and forward them live.

## 2026-10-25

```
wss://relay.damus.io
wss://nos.lol
wss://relay.primal.net
```

The first list: three large, independently run public relays.
