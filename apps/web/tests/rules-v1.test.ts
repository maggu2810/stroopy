// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/rules/v1/*.json
import { describe, expect, test } from "bun:test";
import { CHRONO_TOTAL_MS, itemLimitMs, Mode, roundLimitMs, SplitMix64, TapRoundGenerator, WordItemGenerator } from "../src/core/rules-v1.ts";
import { u64, vectors } from "./vectors.ts";

const v = (f: string) => vectors("rules/v1", f);

describe("rules v1", () => {
  test("splitmix64", async () => {
    for (const c of (await v("splitmix64.json")).cases) {
      const rng = new SplitMix64(u64(c.seed));
      for (const x of c.next) expect(rng.next()).toBe(u64(x));
    }
  });

  test("bounded", async () => {
    for (const c of (await v("bounded.json")).cases) {
      const rng = new SplitMix64(u64(c.seed));
      for (const x of c.values) expect("except" in c ? rng.belowExcept(c.k, c.except) : rng.below(c.k)).toBe(x);
    }
  });

  test("timing", async () => {
    const d = await v("timing.json");
    expect(CHRONO_TOTAL_MS).toBe(d.chronoTotalMs);
    for (const c of d.itemLimit) expect(itemLimitMs(c.n)).toBe(c.ms);
    for (const c of d.roundLimit) expect(roundLimitMs(c.n)).toBe(c.ms);
  });

  for (const [file, mode] of [["items-classic.json", Mode.Classic], ["items-chrono.json", Mode.Chrono], ["items-find.json", Mode.Find]] as const) {
    test(file, async () => {
      const d = await v(file);
      expect(d.modeId).toBe(mode);
      for (const c of d.cases) {
        const gen = new WordItemGenerator(mode, u64(c.seed));
        for (const item of c.items) {
          const { n, pool, ink, word } = gen.next();
          expect({ n, pool, ink, word }).toEqual(item);
        }
      }
    });
  }

  test("rounds-tap", async () => {
    for (const c of (await v("rounds-tap.json")).cases) {
      const gen = new TapRoundGenerator(u64(c.seed));
      for (const round of c.rounds) {
        const { n, pool, inks, words, trueCard } = gen.next();
        expect({ n, pool, inks, words, trueCard }).toEqual(round);
      }
    }
  });
});
