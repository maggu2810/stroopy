# Analysis: original "True Color" 3.0.2 (com.aurelhubert.truecolor)

Study notes from decompiling v3.0.2 (see
`third-party/com.aurelhubert.truecolor/NOTES.md`). These describe *behaviour*
so we can design a compatible-feeling game. Do not copy code or assets.

## Why it probably disappeared

- Online multiplayer was built on **Google Play Games real-time multiplayer**
  (`com.google.android.gms.games.multiplayer.realtime`). Google turned that API
  off in 2020, so the multiplayer feature broke.
- Its target SDK of 27 is far below current Play Store requirements.
- It is full of proprietary SDKs: Firebase Analytics, Crashlytics/Fabric,
  Google Play Billing (a "pro" unlock plus an "international mode" in-app
  purchase), Presage/Ogury ads, GCM push, Realm DB and Picasso.

## Core idea (Stroop effect)

A colour **word** (e.g. "RED") is drawn in an **ink colour**. The player has to
judge or act on whether the word and the ink agree.

### Colour palettes

| set | normal mode | colour-blind mode |
|---|---|---|
| base (always) | yellow `#f1c40f`, red `#e74c3c`, blue `#3498db`, green `#2ecc71` | yellow `#ffd71d`, red `#f95341`, blue `#0a88dc`, green `#18df6c` |
| added during play | orange `#e67e22`, purple `#9b59b6`, pink `#ff89e5`, brown `#a67235` | cyan `#04e5e3`, beige `#cfb891`, purple `#8b0eaa`, brown `#7d4f1b` |

(The normal palette is the common "Flat UI" palette.) The word list is
localised in 9 languages: it, pt, nl, fr, ru, es, en, zh, de. Players can also
pick a language other than the system one ("international mode", paid).

### Generating one item

1. Pick a random ink index from the active pool. It may **not equal the
   previous item's ink**.
2. With 50 % probability the item is *true* (the word names the ink). Otherwise
   the word is a random *different* colour from the pool.
3. Correctness is index-based, so it does not depend on the language.

The pool starts with 4 colours. In Classic mode one extra colour is added when
the player reaches items 20, 30, 40 and 50 (up to 8 colours). In Tap mode the
same happens at scores 5, 10, 15 and 20.

## Game modes

| id | mode | rules |
|---|---|---|
| 1 | **Classic** ("True or false?") | One item at a time with **TRUE / FALSE** buttons and a ring timer around the word. One wrong answer or a timeout ends the game. Score = number of correct answers. |
| 2 | **Chrono** | A **30 s** total timer. Each correct answer is +1. A wrong answer is −1 (never below 0) and vibrates. Score when time runs out. |
| 3 | **Find the true color** | The ink never matches the word. The player taps the colour button for the colour the **word names**. Uses the same per-item ring timer as Classic. |
| 4 | **Tap the true color** | Four words in four distinct inks. Exactly **one** of them matches its ink, and the player taps it. A bar timer runs per round: `2000 − 10·round` ms. A wrong tap or a timeout ends the game. |
| — | **Multiplayer** (2–4 players, Play Games) | See below. |

Every mode starts with a 3-2-1-GO countdown, 1 s per step (Tap mode uses
0.7 s steps).

### Classic speed curve (the "speed increases" part)

For item number `n` (starting at 1):

```
base(n) = 6000 ms           if n == 1
          4000              if n < 10
          3000              if n < 20
          2500              if n < 30
          2000              if n < 40
          1600              otherwise
time(n) = base(n) · (1 − n/300)
ring stroke width = 40 · (1 − n/300)   (the ring also gets thinner)
```

The result is a stepwise drop plus a slow linear squeeze. For example
n = 9 → 3880 ms, n = 10 → 2900 ms, n = 40 → 1733 ms, n = 100 → 1067 ms.

## Original multiplayer

- 2–4 players via a Play Games room (automatch or invite).
- Each player plays a **Chrono-style 30 s** game on their **own random
  sequence**. The sequences are *not* shared.
- A correct answer is +1. A wrong answer is −1 (never below 0) and resets the
  streak.
- Every **10-answer streak** removes **1 s** from all opponents' clocks
  ("remove_time"), with an animation on their screens.
- Messages are small JSON objects broadcast to all peers:
  `{state: playing|finished, score: {participant_id, score},
  action: {remove_time, player}}`.
- Live scores of all players are shown, and the final ranking appears when
  everyone has finished or left.

## Other features

- Local high-score list per mode and language (Realm), plus a "new record"
  dialog with star animation.
- Share score as text through `ACTION_SEND`.
- Tutorial, language picker, colour-blind mode, vibration on failure.
- Fonts: Exo 2 (OFL) and Roboto (Apache 2.0). Both are freely licensed, so we
  can get them from upstream if we want them, never from the APK.

## What we keep, change and add

- **Keep**: the four modes, the speed curve feel, the growing palette,
  colour-blind mode and multiple languages.
- **Change**: generate every sequence from a **seed** with our own specified
  PRNG, so the same seed always produces the same items on every device.
- **Add**: replay or share a sequence, simultaneous multiplayer on an
  identical sequence, and live "who is still alive / at which item" display.
- **Drop**: ads, analytics, billing and every Google dependency.
