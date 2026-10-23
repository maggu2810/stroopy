# 0002: License

Status: accepted (2026-10-19)

> **Note (2026-10-23):** decision 0008 drops F-Droid publication as a
> requirement for now. The licence decision stands unchanged.

## Context
The app must be FOSS and distributable via F-Droid.

## Decision
All our code is licensed **GPL-3.0-or-later** (full text in `LICENSE`).
Source files carry `SPDX-License-Identifier: GPL-3.0-or-later`.

## Consequences
Dependencies must be GPL-3.0-compatible (e.g. Apache-2.0, MIT, BSD, LGPL,
GPL-3.0; fonts under OFL are fine as assets). Copyleft ensures derivatives stay
free.
