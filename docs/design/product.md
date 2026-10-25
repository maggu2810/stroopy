# Product spec

Status: living document. It describes *what* every Stroopy app offers.
Platform idioms (navigation, menus, dialogs) may differ. Interoperability is
defined in `docs/spec/`, not here.

## Screens

| screen | content |
|---|---|
| Home | title; the four modes (classic, chrono, find, tap); entry points to **Multiplayer**, **Highscores**, **Enter code**, **Help** and **Settings** |
| Game | 3-2-1-GO countdown, then the mode's play view (see rules v1); score and item number; quit |
| Result | score, item reached, "new highscore" marker; **Share**, **Play this sequence again**, **New game**, **Home** |
| Highscores | one list per mode (tabs/segmented), best first |
| Enter code | paste or type a game code (or scan a QR code), preview mode, rules version and the sharer's score, then play |
| Settings | language, colour-blind palette, vibration, player name, relays; **Export backup**, **Import backup**, clear highscores |
| Multiplayer | **Create room** or **Join room** (pasted invitation, message or link; scanned QR code later); player name |
| Lobby | **Share invitation** (copy/share, QR code later), the room's relays, connection state (relays connected), the **ranking of the last game** (updating live until everyone is out), players (name, host), mode picker and **Start** for the host; **Leave** |
| Game (multiplayer) | as above, plus a strip with every other player: name, item and score, ✗ once they are out |
| Help | in-app documentation, available offline: how each mode works, the controls for touch, keyboard, gamepad and TV remote, sharing and challenges, and an *About* section (app version, supported rules/game/multiplayer/backup versions, licence, source) |

## Layout

- Every screen supports **portrait and landscape** and switches on the
  window's aspect ratio (not on the device type). This covers phones in
  either orientation, tablets, desktop windows, desktop fullscreen and TVs.
- Landscape game view: the challenge (word with ring timer, or the tap
  cards) on one side, the answer controls on the other. Nothing scrolls
  during a game.
- Content scales with the shorter window edge. Touch targets stay at least
  48 dp on phones.

## Input

All modes are fully playable with **touch/mouse**, **keyboard**, **gamepad**
and a **TV remote** (D-pad + OK + Back). One scheme for all of them: arrows
behave like the D-pad, and keys map to on-screen **positions**, never to
language-specific letters.

### Answer layouts

- **Classic, chrono**: FALSE on the left, TRUE on the right.
- **Tap**: the four cards form a **diamond**: top, right, bottom, left
  (card positions 0, 1, 2, 3).
- **Find**: one button per colour slot at a **fixed position**. Slots 0–3 form
  an inner diamond (top yellow, right red, bottom green, left blue). Slots
  4–7 sit in an outer ring (top-right, bottom-right, bottom-left, top-left)
  and appear as the pool grows.

### Key bindings

| context | keyboard | gamepad | TV remote |
|---|---|---|---|
| classic, chrono | **←** or **F** = FALSE · **→** or **J** = TRUE | D-pad ◀/▶ · **B** = FALSE · **A** = TRUE | ◀ / ▶ |
| tap | **↑ → ↓ ←** or **W D S A** pick the card at that diamond position | D-pad, or face buttons **Y B A X** (top, right, bottom, left) | D-pad |
| find, slots 0–3 | **↑ → ↓ ←** (diamond) | D-pad or **Y B A X** | D-pad |
| find, slots 4–7 | **Shift + ↑ → ↓ ←** = outer button clockwise from top-right | **L1/R1 + direction** | — (see below) |
| find, any slot | **1–8** in slot order (labelled on the buttons) | — | — |
| menus | arrows / Tab move focus, **Enter/Space** activate | D-pad, **A** activate | D-pad, **OK** |
| back / quit game | **Esc** | **B** (outside the game), **Start**/**Select** | **Back** |
| result screen | **R** replay, **N** new game | — | — |
| desktop | **F11** fullscreen | — | — |

Notes:
- Key hints (e.g. `←`, `F`, `1`) appear on the buttons only once a key or
  gamepad button has been used, so touch users don't see them. The system
  back gesture and the volume keys don't count.
- A TV remote has no modifier, so with a remote Find mode is playable up to
  4 colours. Beyond that, the player needs touch, keyboard or gamepad.
  Possible future option: a separate 4-colour mode in a later rules version.
- Fairness between input methods is explicitly **not** a goal. Players
  compare plain scores.

### Platforms

- **Android TV**: listed in the TV launcher (leanback) with a banner. A
  touchscreen is not required. Everything is reachable with the D-pad.
- **Links** on Android and desktop: the app opens `stroopy://play?c=…` (*Enter code*
  with the code) and `stroopy://join?c=…` (the join flow with the
  invitation), whether it is running or not. Nothing starts by itself: the
  player confirms with **Play** or **Join**, which have focus. A link that
  arrives during a game or in a room quits the game and leaves the room
  first. On the desktop, the installed menu entry registers the
  `stroopy` scheme; a link opened while the app runs goes to that window
  instead of starting a second one.
- **Gamepads** on Android: their buttons arrive as key events and follow the
  table above. Analog sticks are not used.
- **Desktop gamepads**: later, e.g. via SDL.
- **Web (PWA)** (decision 0010): installable via the browser's *Add to Home
  Screen* / *Install*; works offline after the first visit. The URL
  parameter `?c=<game code>` opens *Enter code* with the code, `?j=<invitation>`
  opens the join flow. Gamepads via the browser Gamepad API (standard
  mapping), same bindings as the table above. Each open page has a browser
  history entry, so the system back gesture (Android) and the browser's
  back button act like Back; on the home screen they leave the app. The app
  requests persistent storage; the backup code is the safety net.

## Highscores

- Kept **locally** per mode. The top **10** entries are kept, ordered by
  score (descending), then by date (older first).
- An entry stores: mode, score, reached, rules version, seed, date and time,
  and, if the game was started from a received game code, that it was a challenge.
- Every finished game whose score is > 0 and that would enter the top 10 is
  stored automatically. The result screen marks a new **best** score per
  mode.
- Every entry offers:
  - **Share**: produces the game code *including this entry's result*, so
    the receiver sees the score to beat (see `docs/spec/game/`);
  - **Play again**: replays the same sequence.
- Clearing highscores is possible from Settings, after a confirmation.
- Highscores are per rules version in meaning, but are displayed together.
  An entry with an unsupported rules version cannot be replayed.

## Sharing

Every finished game and every highscore entry can be shared as:
- a **text code** (copy to clipboard / platform share sheet), e.g.
  `SYG1-…`, that the receiver pastes in *Enter code*;
- a **QR code** shown on screen and scanned in *Enter code*;
- optionally a short message around it ("I scored 47 in Stroopy Classic:
  SYG1-…").

The app also accepts a pasted message that *contains* a code: it extracts
the first valid code.

## Backup

Highscores and settings can be moved to another device or another Stroopy
app with one **backup code** (`docs/spec/backup/v1.md`,
decision 0011).

- **Export backup** (Settings) shows the code with **Copy** and, where the
  platform has one, **Share**. It always contains all highscores and all
  shared settings.
- **Import backup** (Settings) takes pasted text (the code may be embedded
  in a message or wrapped over lines), shows what it contains (number of
  highscores per mode, whether settings are included), and offers
  **Import** with a switch **Also restore settings** (on by default).
- Highscores are **merged**: imported entries are added, exact duplicates
  dropped, and each mode keeps its top 10. Nothing is deleted.
- An invalid code or an unsupported version shows an error and changes
  nothing.

## Settings

| setting | values | default |
|---|---|---|
| Language | *Follow system*, English, Deutsch (more later) | Follow system |
| Colour-blind palette | on / off | off |
| Vibration on failure | on / off | on |
| Player name | 1–24 characters, shown to other players | `Player` + 3 random digits, chosen once |
| Nostr relays | list of `wss://` URLs, one per line, used for rooms the player creates (at most 8 are used); empty = the app's default relays (`docs/design/nostr-relays.md`) | empty |

- *Follow system* uses the OS UI language if it is supported, and English
  otherwise.
- A language change applies **immediately**, without a restart. This includes
  the colour words in the game.
- Where the OS has a per-app language (Android 13+ "App language"), the
  setting **is** that system setting: a change on either side shows on the
  other, and *Follow system* means no per-app language.
- Settings persist across app starts.

## Multiplayer

Live games over public Nostr relays (`docs/spec/multiplayer/v1.md`).
Network access happens only while the player is in a room.

- **Create room**: a new room id on the relays from the settings (or the
  default relays); the creator is host. The lobby offers **Share
  invitation**: **Copy**/**Share** with a short message ("Play Stroopy with
  me: stroopy://join?c=SYM1-…"), and later a QR code for players nearby.
  Every player in the room can share it, not only the host. The room id is
  not shown on its own, because it can't be used to join.
- **Join room**: paste an invitation (also inside a message, wrapped over
  lines, or as a link), later also scan its QR code. A link `stroopy://join?c=…` or
  the web app URL with `?j=…` opens the join flow directly. The guest uses
  the room's relays from the invitation; its own relay setting doesn't
  matter. An invitation from a newer app that offers no transport this app
  supports shows "this room needs a newer version".
- The player name is edited on the Multiplayer screen and stored in the
  settings. A note says that names and scores travel unencrypted through
  public relays.
- **Lobby**: players with a host marker, the room's relays and their status
  (e.g. "2/3 relays"), the host's mode selection (guests see it read-only).
  The host's **Start** is enabled when at least one relay is connected.
- **Game**: everyone gets the 3-2-1-GO countdown on receiving the start, so
  starts differ by network delay. Fairness across latency is not a goal.
- **Results**: when the own game is over, the lobby comes back with the
  ranking of that game; it keeps updating until all players are out (or
  gone). Players who leave keep their result in that ranking. Multiplayer
  games enter the local highscores like any other game.
- A guest who arrives while a game runs joins it at once (same sequence,
  later start).
- Quitting a running game (Back/Esc) counts as being out at the current
  item; the player stays in the room. Back in the lobby leaves the room.

## Languages

At least **English** and **German**. Every user-visible string is
translatable, including colour words. Colour words are uppercase in all
languages.

| slot | en | de |
|---|---|---|
| 0 | YELLOW | GELB |
| 1 | RED | ROT |
| 2 | BLUE | BLAU |
| 3 | GREEN | GRÜN |
| 4 | ORANGE (cb: CYAN) | ORANGE (cb: CYAN) |
| 5 | PURPLE (cb: BEIGE) | LILA (cb: BEIGE) |
| 6 | PINK (cb: PURPLE) | ROSA (cb: LILA) |
| 7 | BROWN | BRAUN |
