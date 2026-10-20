// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/backup/v1/*.json
import { describe, expect, test } from "bun:test";
import { decodeText, encodeRecord, encodeText, type Backup, type Settings } from "../src/core/backup-v1.ts";
import { hex, u64, vectors } from "./vectors.ts";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function backup(c: any, clamp = false): Backup {
  const f = (x: number) => (clamp ? Math.min(x, 0xffff) : x);
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const b: Backup = { entries: c.entries.map((e: any) => ({ ...e, seed: u64(e.seed), score: f(e.score), reached: f(e.reached) })) };
  if (c.settings) {
    const s: Settings = {};
    if ("1" in c.settings) s.language = c.settings["1"];
    if ("2" in c.settings) s.colourBlind = c.settings["2"];
    if ("3" in c.settings) s.vibration = c.settings["3"];
    if ("4" in c.settings) s.name = c.settings["4"];
    if ("5" in c.settings) s.relays = c.settings["5"];
    b.settings = s;
  }
  return b;
}

describe("backup v1", () => {
  test("encode", async () => {
    for (const c of (await vectors("backup/v1", "encode.json")).cases) {
      expect(hex(encodeRecord(backup(c))), c.note).toBe(c.record);
      expect(encodeText(backup(c)), c.note).toBe(c.text);
      expect(decodeText(c.text), c.note).toEqual({ ok: true, value: backup(c, true) });
    }
  });

  test("decode", async () => {
    const d = await vectors("backup/v1", "decode.json");
    for (const c of d.valid) expect(decodeText(c.input), c.note).toEqual({ ok: true, value: backup(c.expected) });
    for (const c of d.errors) {
      const r = decodeText(c.input);
      expect(r.ok, c.note).toBe(false);
      if (!r.ok) {
        expect(r.kind, c.note).toBe(c.error);
        if (r.kind === "unsupported") expect(r.version).toBe(c.version);
      }
    }
  });
});
