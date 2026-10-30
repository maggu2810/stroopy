# Visual style

Status: living document. It describes how every Stroopy app *looks*, so that
all apps share one appearance and an app can be (re)built from this page
alone. Screens, flows and input are in `product.md`; the colour slots and
words of the game are in `docs/spec/rules/v1.md`, appendix A.

Like `product.md`, this page is not an interop spec: an app may follow a
platform idiom where this page says so (marked *idiom*), but otherwise it
follows the values below. No app is the reference; when an app and this page
disagree, fix the page first (if the app is better) or the app.

Sizes are logical pixels: CSS `px` in the web app, device-independent pixels
(dp) on Android, logical pixels in Qt.

## Colours

Dark theme only.

| token | value | use |
|---|---|---|
| background | `#1d2330` | page background; text on accent, ink and answer fills |
| surface | `#2a3242` | buttons, sub page bar, tap cards, time bar track, separators |
| surface-high | `#364056` | Multiplayer button on Home, switch track (off), toasts |
| text | `#f2f4f8` | normal text; time bar fill; checked choice buttons |
| muted | `#9aa4b5` | secondary text, item number, inactive tabs, definitions |
| accent | `#f1c40f` | primary buttons, headings on Help, best score, new best, active tab, switch (on) |
| good | `#2ecc71` | TRUE button, correct-answer flash, beaten challenge |
| bad | `#e74c3c` | FALSE button, wrong-answer flash, destructive actions, errors |
| field border | `#4a556b` | 1 px border of text fields |

Ink colours (words, rings, Find buttons, Home mode cards): the normal and
colour-blind palettes of the rules spec. The four Home mode cards use slots
0–3 in mode order (Classic yellow, Chrono red, Find blue, Tap green).

## Typography

The platform's sans-serif system font. Codes (game, backup) use the
platform's monospace font.

| element | size | weight | colour |
|---|---|---|---|
| body text, text fields | 16 | regular | text |
| small notes (field hints, dates, mode hints) | 13 | regular (mode hints medium) | muted |
| button labels | 16 | semi-bold | see buttons |
| section heading | 20 | bold | text |
| Help section heading | 20 | bold | accent |
| sub page title | 20 | bold | text |
| app title "STROOPY" (Home) | 48 | bold, letter spacing 4 | ink slot 2 (blue) |
| mode card name / hint | 20 bold / 13 | | background, hint at 80 % opacity |
| best score on a mode card | 18 | bold | background |
| game score / item number | 32 bold / 18 | | text / muted |
| challenge word | 64, shrinking to fit 80 % of the ring | extra bold | ink |
| tap card word | 30, shrinking to fit | extra bold | ink |
| countdown 3-2-1-GO | 120 | bold | text |
| result score | 96 | extra bold | accent for a new best, else text |
| result mode / item reached | 18 | regular | muted |
| result rank ("New best!") | 22 | bold | accent |
| key hints on buttons | 12 (11 on tap cards) | semi-bold | at 70 % opacity |
| code fields | 13 | monospace | text |

## Shapes

- Corner radius **14**: mode cards, the Multiplayer button on Home, answer
  buttons, Find buttons, tap cards.
- **Pill** (fully rounded): all other buttons.
- Radius **10**: the mode choice buttons in the lobby.
- Radius **8**: text fields.
- No shadows or elevation anywhere.

## Controls

**Buttons** are at least 48 high with 20 padding left and right, 16
semi-bold labels and no shadow.

| kind | fill | label |
|---|---|---|
| default | surface | text |
| primary (the main action of a screen) | accent | background |
| flat (links, secondary actions) | none | text; bad for destructive ones ("Clear highscores", "Leave room") |
| checked (a chosen option) | text | background |
| tall (Play, Create room, Start, Join the running game) | as primary, 64 high | |

Disabled buttons keep their colours at 40 % opacity. Hover lightens and
pressing darkens the fill slightly. *Idiom*: a platform's own press
feedback (e.g. a ripple) is fine.

**Text fields** are at least 48 high with a 1 px field-border outline,
radius 8 and no fill. *Idiom*: a platform's outlined field style (e.g.
Material's floating label) is fine as long as it stays outlined and unfilled.

**Switches** sit at the left edge of the column, label to the right. Track
52 × 32, off: surface-high with a muted knob; on: accent with a background
knob. *Idiom*: a platform's own switch in these colours.

**Tabs** (Highscores): no fill, labels 16 semi-bold, inactive muted, active
accent with an accent underline. Tabs show the full mode names and
scroll sideways when they don't fit.

**Toasts** (e.g. "copied"): surface-high, radius 10, centred near the bottom.
*Idiom*: a platform's own toast or snackbar.

**Focus frame**: once a key or gamepad button has been used, the focused
control gets a 3 px text-colour frame 3 px outside it. Touch users never see
it (see `product.md`, "Input").

## Page frame

- **Sub pages** (everything except Home, Game and Result) have a bar at the
  top: 56 high, surface fill, a back button "‹" (26–28) and the title.
- Content scrolls below the bar with 24 padding above and below and 16 left
  and right, in a centred **column** at most 560 wide; items in a column are
  14 apart.
- **Two-column pages** (Backup, Lobby, Result) use one column of at most 420
  in portrait and two side by side, at most 1100 together with 48 between
  them, in landscape.

## Screens

**Home**: title and tagline centred above the menu in portrait, on the left
of it in landscape (48 between). The menu is at most 420 wide in portrait
and 960 for both parts in landscape, 24 from the window edges:

- the four mode cards (72 high, ink fill, name and hint left-aligned, best
  score right), one per row in portrait, two per row in landscape, 12 apart;
- the full-width Multiplayer button;
- flat links Highscores, Enter code, Help, Settings: two per row in portrait,
  four in one row in landscape.

**Game**: 16 from the window edges, 12 between the rows.

- Top row: quit "×" left (as large as a 26 `✕`), then right-aligned the
  item number "#n" (18 muted) and the score (32 bold).
- Multiplayer: a strip of the other players (15) below the top row.
- Chrono and Tap: a time bar below, 10 high, fully rounded, surface track,
  text-colour fill draining from right to left.
- Word modes: the word centred in the **ring timer**, a square as large as
  the space allows. The ring's stroke is 7 % of its size and thins with the
  item number `n` to `max(2 %, 7 % × (1 − n / 300))`; round caps; the
  arc in the ink colour on a track of the same colour at 15 % opacity. It
  starts as a full circle at the top and its end runs back counter-clockwise
  to the top until the item's time is up.
- Classic and Chrono: FALSE (bad) and TRUE (good) side by side, 16 apart,
  radius 14, labels 24 extra bold in the background colour. Portrait: a
  110-high row at the bottom. Landscape: the ring on the left half, the two
  buttons filling the right half.
- Find: the colour buttons in a square 3 × 3 grid (10 apart) at the
  positions of `product.md`, "Answer layouts".
- Tap: the four cards (surface fill) as a diamond in a square 3 × 3 grid.
- After each answer the whole screen flashes good or bad, from 25 %
  opacity to none in 250 ms.

**Result**: in portrait a centred column (score block above the actions),
in landscape the score block left and the actions right. Score block,
centred: mode, score, rank, item reached, challenge verdict, 8 apart.
Actions, full width, 8 apart: the game code in a read-only centred code
field, Share, then (16 further down) Play this sequence again, New game
(primary), Home (flat).

**Highscores**: the tabs under the bar, then one row per entry, at least 64
high with a surface separator: position (18 muted), score (26 extra bold,
accent for the best), "out at #n" with the date and time below (13 muted,
short format of the app language), and flat Share and ▶ buttons.

**Help**: the column with accent section headings. Term and definition lists
show the term (bold; key names monospace) left and the definition (muted)
right, 16 apart; at 480 wide or less the definition goes below its term.

**Enter code, Multiplayer, Settings, Backup**: a column of the controls
listed in `product.md`; the screen's main action is a tall primary button
right below the inputs.
