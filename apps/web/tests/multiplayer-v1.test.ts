// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/multiplayer/v1/*.json
import { describe, expect, test } from "bun:test";
import { hexToBytes } from "@noble/hashes/utils.js";
import { createEvent, decodeInvitation, encodeRecord, encodeText, parseMessage, roomIdFromValue, roomTag, serialise, verifyEvent } from "../src/net/multiplayer-v1.ts";
import { hex, u64, vectors } from "./vectors.ts";

const v = (f: string) => vectors("multiplayer/v1", f);

describe("multiplayer v1", () => {
  test("invitations", async () => {
    const d = await v("invitations.json");
    for (const c of d.encode) {
      expect(roomIdFromValue(u64(c.room)), c.note).toBe(c.roomId);
      const inv = { room: c.roomId, relays: c.relays };
      expect(hex(encodeRecord(inv)), c.note).toBe(c.record);
      expect(encodeText(inv), c.note).toBe(c.text);
      expect(roomTag(c.roomId), c.note).toBe(c.tag);
      expect(decodeInvitation(c.text), c.note).toEqual({ ok: true, value: inv });
    }
    for (const c of d.parse) expect(decodeInvitation(c.input), c.note).toEqual({ ok: true, value: c.expected });
    for (const c of d.errors) {
      const r = decodeInvitation(c.input);
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
