// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/protocol/v1/*.json
import { describe, expect, test } from "bun:test";
import { hexToBytes } from "@noble/hashes/utils.js";
import { createEvent, parseMessage, parseRoom, roomCode, roomIdFromValue, roomTag, serialise, verifyEvent } from "../src/net/protocol-v1.ts";
import { u64, vectors } from "./vectors.ts";

const v = (f: string) => vectors("protocol/v1", f);

describe("protocol v1", () => {
  test("rooms", async () => {
    const d = await v("rooms.json");
    for (const c of d.encode) {
      expect(roomIdFromValue(u64(c.value))).toBe(c.id);
      expect(roomCode(c.id)).toBe(c.code);
      expect(roomTag(c.id)).toBe(c.tag);
    }
    for (const c of d.parse) expect(parseRoom(c.input), c.note).toEqual({ ok: true, value: c.expected });
    for (const c of d.errors) {
      const r = parseRoom(c.input);
      expect(r.ok, c.note).toBe(false);
      if (!r.ok) {
        expect(r.kind, c.note).toBe(c.error);
        if (r.kind === "unsupported") expect(r.version).toBe(c.version);
      }
    }
  });

  test("events", async () => {
    const d = await v("events.json");
    for (const c of d.sign) {
      const e = createEvent(hexToBytes(c.secretKey), c.createdAt, c.roomId, c.content, hexToBytes(d.auxRand));
      expect(serialise(e.pubkey, e.created_at, e.kind, e.tags, e.content)).toBe(c.serialised);
      expect(e).toEqual(c.event);
      expect(verifyEvent(c.event)).toBe(true);
    }
    for (const c of d.reject) expect(verifyEvent(c.event), c.note).toBe(false);
  });

  test("messages", async () => {
    for (const c of (await v("messages.json")).cases) {
      const m = parseMessage(c.content);
      if (m?.type === "start") {
        const { type, rules, mode, seed } = m;
        expect({ type, rules, mode, seed: "0x" + seed.toString(16).padStart(16, "0") }, c.note).toEqual(c.expected);
      } else expect(m, c.note).toEqual(c.expected);
    }
  });
});
