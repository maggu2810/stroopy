# 0002: License

Status: accepted (2026-10-19)

## Context
The app must be free and open source, run on de-Googled phones and stay
eligible for F-Droid, even though publishing there is not planned for now
(decision 0008).

## Decision
All our code is licensed **GPL-3.0-or-later** (full text in `LICENSE`).
Source files carry `SPDX-License-Identifier: GPL-3.0-or-later`.

## Consequences
Dependencies must be GPL-3.0-compatible (e.g. Apache-2.0, MIT, BSD, LGPL,
GPL-3.0; fonts under OFL are fine as assets) and must not pull in
proprietary SDKs such as Google Play Services or Firebase. Copyleft ensures
derivatives stay free.
