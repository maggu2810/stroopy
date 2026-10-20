// SPDX-License-Identifier: GPL-3.0-or-later
// Local highscores per mode (docs/design/product.md, "Highscores").
import { signal } from "@preact/signals";
import { load, save } from "./storage.ts";

export const MAX_ENTRIES_PER_MODE = 10;

export interface Highscore {
  mode: number;
  rules: number;
  seed: string; // 16 hex digits
  score: number;
  reached: number;
  time: number; // Unix seconds
  challenge: boolean;
}

export const highscores = signal<Highscore[]>(load("highscores", []));

function store(list: Highscore[]) {
  highscores.value = list;
  save("highscores", list);
}

// Better first: higher score, then earlier date.
const better = (a: Highscore, b: Highscore) => b.score - a.score || a.time - b.time;

const same = (a: Highscore, b: Highscore) =>
  a.mode === b.mode && a.rules === b.rules && BigInt("0x" + a.seed) === BigInt("0x" + b.seed) &&
  a.score === b.score && a.reached === b.reached && a.time === b.time && a.challenge === b.challenge;

export function entries(mode: number): Highscore[] {
  return highscores.value.filter((e) => e.mode === mode).sort(better);
}

export function best(mode: number): number {
  return entries(mode)[0]?.score ?? 0;
}

/** Keeps the top list of every mode. */
function trimmed(list: Highscore[]): Highscore[] {
  const modes = [...new Set(list.map((e) => e.mode))];
  return modes.flatMap((m) => list.filter((e) => e.mode === m).sort(better).slice(0, MAX_ENTRIES_PER_MODE));
}

/** Stores the game if it makes the top list; returns its 1-based rank or 0. */
export function submit(entry: Omit<Highscore, "time">): number {
  if (entry.score <= 0) return 0;
  const e: Highscore = { ...entry, time: Math.floor(Date.now() / 1000) };
  const list = trimmed([...highscores.value, e]);
  if (!list.includes(e)) return 0;
  store(list);
  return entries(e.mode).indexOf(e) + 1;
}

/** Adds entries, drops exact duplicates and keeps the top lists; returns how many were added and kept. */
export function merge(incoming: Highscore[]): number {
  const added: Highscore[] = [];
  for (const e of incoming) {
    if (e.score > 0 && ![...highscores.value, ...added].some((x) => same(x, e))) added.push(e);
  }
  if (!added.length) return 0;
  const list = trimmed([...highscores.value, ...added]);
  store(list);
  return added.filter((e) => list.includes(e)).length;
}

export function clear() {
  store([]);
}
