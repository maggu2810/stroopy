// SPDX-License-Identifier: GPL-3.0-or-later
// Rules v1 (docs/spec/rules/v1.md): PRNG, pool, item generation and time limits.

export const RULES_VERSION = 1;
export const CHRONO_TOTAL_MS = 30000;

export const Mode = { Classic: 1, Chrono: 2, Find: 3, Tap: 4 } as const;
export type Mode = (typeof Mode)[keyof typeof Mode];
export const MODES: readonly Mode[] = [Mode.Classic, Mode.Chrono, Mode.Find, Mode.Tap];

export function modeFromId(id: number): Mode | undefined {
  return MODES.find((m) => m === id);
}

const MASK = (1n << 64n) - 1n;

export class SplitMix64 {
  private state: bigint;

  constructor(seed: bigint) {
    this.state = seed & MASK;
  }

  next(): bigint {
    this.state = (this.state + 0x9e3779b97f4a7c15n) & MASK;
    let z = this.state;
    z = ((z ^ (z >> 30n)) * 0xbf58476d1ce4e5b9n) & MASK;
    z = ((z ^ (z >> 27n)) * 0x94d049bb133111ebn) & MASK;
    return z ^ (z >> 31n);
  }

  /** Uniform value in [0, k), 1 <= k <= 2^32 (spec §2.1). */
  below(k: number): number {
    const kk = BigInt(k);
    const threshold = ((1n << 64n) - kk) % kk;
    for (;;) {
      const x = this.next();
      if (x >= threshold) return Number(x % kk);
    }
  }

  /** Uniform value in [0, k) without e, k >= 2 (spec §2.2). */
  belowExcept(k: number, e: number): number {
    const r = this.below(k - 1);
    return r >= e ? r + 1 : r;
  }

  coin(): boolean {
    return this.below(2) === 1;
  }
}

const WORD_THRESHOLDS = [20, 30, 40, 50];
const TAP_THRESHOLDS = [6, 11, 16, 21];

export function poolSize(n: number, mode: Mode): number {
  const t = mode === Mode.Tap ? TAP_THRESHOLDS : WORD_THRESHOLDS;
  return 4 + t.filter((x) => x <= n).length;
}

export function itemLimitMs(n: number): number {
  const base = n === 1 ? 6000 : n < 10 ? 4000 : n < 20 ? 3000 : n < 30 ? 2500 : n < 40 ? 2000 : 1600;
  return Math.max(250, Math.trunc((base * (300 - n)) / 300));
}

export function roundLimitMs(n: number): number {
  return Math.max(250, 2000 - 10 * (n - 1));
}

export interface WordItem {
  kind: "word";
  n: number;
  pool: number;
  ink: number;
  word: number;
}

export interface TapRound {
  kind: "tap";
  n: number;
  pool: number;
  inks: number[];
  words: number[];
  trueCard: number;
}

export class WordItemGenerator {
  private rng: SplitMix64;
  private n = 0;
  private prevInk = 0;

  constructor(
    private readonly mode: Mode,
    seed: bigint,
  ) {
    this.rng = new SplitMix64(seed);
  }

  next(): WordItem {
    const n = ++this.n;
    const p = poolSize(n, this.mode);
    const ink = n === 1 ? this.rng.below(p) : this.rng.belowExcept(p, this.prevInk);
    let word: number;
    if (this.mode === Mode.Find) word = this.rng.belowExcept(p, ink);
    else word = this.rng.coin() ? ink : this.rng.belowExcept(p, ink);
    this.prevInk = ink;
    return { kind: "word", n, pool: p, ink, word };
  }
}

export class TapRoundGenerator {
  private rng: SplitMix64;
  private n = 0;

  constructor(seed: bigint) {
    this.rng = new SplitMix64(seed);
  }

  next(): TapRound {
    const n = ++this.n;
    const p = poolSize(n, Mode.Tap);
    const a = Array.from({ length: p }, (_, i) => i);
    for (let i = 0; i < 4; ++i) {
      const j = i + this.rng.below(p - i);
      [a[i], a[j]] = [a[j]!, a[i]!];
    }
    const inks = a.slice(0, 4);
    const trueCard = this.rng.below(4);
    const words = inks.map((ink, c) => (c === trueCard ? ink : this.rng.belowExcept(p, ink)));
    return { kind: "tap", n, pool: p, inks, words, trueCard };
  }
}
