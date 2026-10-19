# Product spec

Status: living document. It describes *what* every Stroopy app offers.
Platform idioms (navigation, menus, dialogs) may differ. Interoperability is
defined in `docs/spec/`, not here.

## Screens

| screen | content |
|---|---|
| Home | title; the four modes (classic, chrono, find, tap); entry points to **Highscores**, **Enter code** and **Settings** |
| Game | 3-2-1-GO countdown, then the mode's play view (see rules v1); score and item number; quit |
| Result | score, item reached, "new highscore" marker; **Share**, **Play this sequence again**, **New game**, **Home** |
| Highscores | one list per mode (tabs/segmented), best first |
| Enter code | paste or type a share code (or scan a QR code), preview mode, rules version and the sharer's score, then play |
| Settings | language, colour-blind palette, vibration |

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
  `SY1-…`, that the receiver pastes in *Enter code*;
- a **QR code** shown on screen and scanned in *Enter code*;
- optionally a short message around it ("I scored 47 in Stroopy Classic:
  SY1-…").

The app also accepts a pasted message that *contains* a code: it extracts
the first valid code.

## Settings

| setting | values | default |
|---|---|---|
| Language | *Follow system*, English, Deutsch (more later) | Follow system |
| Colour-blind palette | on / off | off |
| Vibration on failure | on / off | on |

- *Follow system* uses the OS UI language if it is supported, and English
  otherwise.
- A language change applies **immediately**, without a restart. This includes
  the colour words in the game.
- Settings persist across app starts.

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
