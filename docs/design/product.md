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
| Enter code | paste or type a share code (or scan a QR code), preview mode, rules version and the sharer's score, then play |
| Settings | language, colour-blind palette, vibration, player name, relays; **Export backup**, **Import backup**, clear highscores |
| Multiplayer | **Create room** or **Join room** (room code, pasted message or link); player name |
| Lobby | room code (copy/share, QR later), connection state (relays connected), the **ranking of the last game** (updating live until everyone is out), players (name, host), mode picker and **Start** for the host; **Leave** |
| Game (multiplayer) | as above, plus a strip with every other player: name, item and score, ✗ once they are out |
| Help | in-app documentation, available offline: how each mode works, the controls for touch, keyboard, gamepad and TV remote, sharing and challenges, and an *About* section (app version, supported rules/share-code/protocol versions, licence, source) |

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
  gamepad button has been used, so touch users don't see them.
- A TV remote has no modifier, so with a remote Find mode is playable up to
  4 colours. Beyond that, the player needs touch, keyboard or gamepad.
  Possible future option: a separate 4-colour mode in a later rules version.
- Fairness between input methods is explicitly **not** a goal. Players
  compare plain scores.

### Platforms

- **Android TV**: listed in the TV launcher (leanback) with a banner. A
  touchscreen is not required. Everything is reachable with the D-pad.
- **Gamepads** on Android: their buttons arrive as key events and follow the
  table above. Analog sticks are not used.
- **Desktop gamepads**: later, e.g. via SDL.
- **Web (PWA)** (decision 0010): installable via the browser's *Add to Home
  Screen* / *Install*; works offline after the first visit. The URL
  parameter `?c=<share code>` opens *Enter code* with the code, `?r=<room
  code>` opens the join flow. Gamepads via the browser Gamepad API (standard
  mapping), same bindings as the table above. The app requests persistent
  storage; the backup code is the safety net.

## Highscores

- Kept **locally** per mode. The top **10** entries are kept, ordered by
  score (descending), then by date (older first).
- An entry stores: mode, score, reached, rules version, seed, date and time,
  and, if the game was started from a share code, that it was a challenge.
- Every finished game whose score is > 0 and that would enter the top 10 is
  stored automatically. The result screen marks a new **best** score per
  mode.
- Every entry offers:
  - **Share**: produces the share code *including this entry's result*, so
    the receiver sees the score to beat (see `docs/spec/share-code/`);
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
| Nostr relays | list of `wss://` URLs, one per line; empty = default relays (protocol v1 §3.4) | empty |

- *Follow system* uses the OS UI language if it is supported, and English
  otherwise.
- A language change applies **immediately**, without a restart. This includes
  the colour words in the game.
- Settings persist across app starts.

## Multiplayer

Live games over public Nostr relays (`docs/spec/protocol/v1.md`). Network
access happens only while the player is in a room.

- **Create room**: a new room id, the creator is host. The lobby shows the
  room code and a **Copy**/**Share** action with a short message
  ("Play Stroopy with me: SYR1-…").
- **Join room**: paste or type a room code (also inside a message or link).
  A link `stroopy://join?r=…` or the web app URL with `?r=…` opens the join
  flow directly.
- The player name is edited on the Multiplayer screen and stored in the
  settings. A note says that names and scores travel unencrypted through
  public relays.
- **Lobby**: players with a host marker, a relay status (e.g. "2/3 relays"),
  the host's mode selection (guests see it read-only). The host's **Start**
  is enabled when at least one relay is connected.
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
