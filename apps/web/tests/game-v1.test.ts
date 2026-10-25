// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/game/v1/*.json
import { describe, expect, test } from "bun:test";
import { crc16 } from "../src/core/codec.ts";
import { decodeText, encodeRecord, encodeText, type Descriptor } from "../src/core/game-v1.ts";
import { hex, u64, vectors } from "./vectors.ts";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function descriptor(c: any, clamp = false): Descriptor {
  const d: Descriptor = { rules: c.rules, mode: c.mode, seed: u64(c.seed) };
  if (c.result) {
    const f = (x: number) => (clamp ? Math.min(x, 0xffff) : x);
    d.result = { score: f(c.result.score), reached: f(c.result.reached) };
  }
  return d;
}

describe("game v1", () => {
  test("crc", async () => {
    const d = await vectors("game/v1", "encode.json");
    expect(crc16(new TextEncoder().encode(d.crc16Check.input))).toBe(Number(d.crc16Check.crc));
  });

  test("encode", async () => {
    for (const c of (await vectors("game/v1", "encode.json")).cases) {
      expect(hex(encodeRecord(descriptor(c)))).toBe(c.record);
      expect(encodeText(descriptor(c))).toBe(c.text);
      expect(decodeText(c.text)).toEqual({ ok: true, value: descriptor(c, true) });
    }
  });

  test("decode", async () => {
    const d = await vectors("game/v1", "decode.json");
    for (const c of d.valid) expect(decodeText(c.input), c.note).toEqual({ ok: true, value: descriptor(c.expected) });
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
