// SPDX-License-Identifier: GPL-3.0-or-later
// Multiplayer v1 (docs/spec/multiplayer/v1.md): invitations, messages and Nostr events.
import { schnorr } from "@noble/curves/secp256k1.js";
import { sha256 } from "@noble/hashes/sha2.js";
import { bytesToHex, hexToBytes } from "@noble/hashes/utils.js";
import { ALPHABET, base32, ByteWriter, crc16, getU16, grouped, invalid, normalise, unbase32, unsupported, type Decoded } from "../core/codec.ts";
import { decodeText } from "../core/game-v1.ts";

export const MULTIPLAYER_VERSION = 1;
export const KIND = 25713;

// ---- invitations (§1) ----

const TAG_NOSTR = 1;
export const MAX_RELAYS = 8;

export interface Invitation {
  /** Room id as 8 Crockford characters. */
  room: string;
  relays: string[];
}

/** 40-bit value -> 8 Crockford characters. */
export function roomIdFromValue(value: bigint): string {
  let id = "";
  for (let i = 0; i < 8; i++) id += ALPHABET[Number((value >> BigInt(35 - 5 * i)) & 31n)];
  return id;
}

export function randomRoomId(): string {
  const b = crypto.getRandomValues(new Uint8Array(5));
  return roomIdFromValue(b.reduce((v, x) => (v << 8n) | BigInt(x), 0n));
}

export function roomTag(id: string): string {
  return `stroopy-m${MULTIPLAYER_VERSION}-${id.toLowerCase()}`;
}

const utf8 = new TextEncoder();
const strictUtf8 = new TextDecoder("utf-8", { fatal: true });

/** True if `url` may be a relay entry (§1.1): ws(s) scheme, no white space or control characters, at most 255 bytes. */
export function isRelayUrl(url: string): boolean {
  // eslint-disable-next-line no-control-regex
  return /^wss?:\/\/[^\u0000-\u0020\u007f]+$/.test(url) && utf8.encode(url).length <= 255;
}

export function encodeRecord(inv: Invitation): Uint8Array {
  const w = new ByteWriter().u8(MULTIPLAYER_VERSION).raw(unbase32(inv.room).bytes).u8(inv.relays.length);
  for (const url of inv.relays) {
    const value = utf8.encode(url);
    w.u8(TAG_NOSTR).u8(value.length).raw(value);
  }
  return w.crc().result();
}

export function encodeText(inv: Invitation): string {
  return `SYM${MULTIPLAYER_VERSION}-` + grouped(base32(encodeRecord(inv).subarray(1)));
}

class TooShort extends Error {}
class Bad extends Error {}

const PREFIX = /SYM([0-9]+)-/i;

/** Finds and decodes the first invitation in arbitrary text (§1.2). */
export function decodeInvitation(text: string): Decoded<Invitation> {
  const m = PREFIX.exec(text);
  if (!m) return invalid;
  const version = Number(m[1]);
  if (version !== MULTIPLAYER_VERSION) return unsupported(version);

  let chars = "";
  for (const raw of text.slice(m.index + m[0].length)) {
    if (raw === "-" || /\s/.test(raw)) continue;
    const c = normalise(raw);
    if (!c) break;
    chars += c;
  }
  const data = Uint8Array.from([MULTIPLAYER_VERSION, ...unbase32(chars).bytes]);
  let pos = 1;
  const need = (n: number) => {
    if (pos + n > data.length) throw new TooShort();
  };
  const u8 = () => (need(1), data[pos++]!);

  try {
    need(5);
    const room = base32(data.subarray(1, 6));
    pos = 6;
    const count = u8();
    if (count === 0) throw new Bad();
    const relays: string[] = [];
    for (let i = 0; i < count; ++i) {
      const tag = u8();
      const len = u8();
      need(len);
      const value = data.subarray(pos, pos + len);
      pos += len;
      if (tag >= 128) throw new Bad();
      if (tag !== TAG_NOSTR) continue; // unknown transport: skipped
      let url: string;
      try {
        url = strictUtf8.decode(value);
      } catch {
        throw new Bad();
      }
      if (!isRelayUrl(url) || relays.includes(url) || relays.length === MAX_RELAYS) throw new Bad();
      relays.push(url);
    }
    const crcPos = pos;
    need(2);
    // Exactly the characters needed for the record belong to the code (§1.2 step 4).
    const length = crcPos + 2;
    const needed = Math.ceil(((length - 1) * 8) / 5);
    const pad = needed * 5 - (length - 1) * 8;
    if ((ALPHABET.indexOf(chars[needed - 1]!) & ((1 << pad) - 1)) !== 0) return invalid;
    if (crc16(data.subarray(0, crcPos)) !== getU16(data, crcPos)) return invalid;
    if (!relays.length) return { ok: false, kind: "noTransport" };
    return { ok: true, value: { room, relays } };
  } catch (e) {
    if (e instanceof TooShort || e instanceof Bad) return invalid;
    throw e;
  }
}

// ---- messages (§2) ----

export interface GameState {
  code: string;
  n: number;
  score: number;
  over: boolean;
}

export type Message =
  | { type: "status"; name: string; host: boolean; rules: number[]; mode?: number; game?: GameState }
  | { type: "start"; code: string; rules: number; mode: number; seed: bigint }
  | { type: "bye" };

const MAX_NAME_LENGTH = 24;

export function cleanName(name: string): string {
  // eslint-disable-next-line no-control-regex
  const s = [...name.replace(/[\u0000-\u001f\u007f]/g, "").trim()].slice(0, MAX_NAME_LENGTH).join("");
  return s || "?";
}

const isInt = (v: unknown, lo: number, hi = Infinity): v is number => typeof v === "number" && Number.isInteger(v) && v >= lo && v <= hi;
const isObject = (v: unknown): v is Record<string, unknown> => typeof v === "object" && v !== null && !Array.isArray(v);

/** Event content -> message, or null if it must be ignored (§2). */
export function parseMessage(content: string): Message | null {
  let m: unknown;
  try {
    m = JSON.parse(content);
  } catch {
    return null;
  }
  if (!isObject(m) || m.m !== MULTIPLAYER_VERSION) return null;
  switch (m.type) {
    case "bye":
      return { type: "bye" };
    case "start": {
      if (typeof m.code !== "string") return null;
      const d = decodeText(m.code);
      if (!d.ok || d.value.result) return null;
      return { type: "start", code: m.code, rules: d.value.rules, mode: d.value.mode, seed: d.value.seed };
    }
    case "status": {
      const { name, host, rules, mode, game } = m;
      if (typeof name !== "string" || typeof host !== "boolean") return null;
      if (!Array.isArray(rules) || !rules.length || !rules.every((r) => isInt(r, 1, 255))) return null;
      const out: Message = { type: "status", name: cleanName(name), host, rules };
      if ("mode" in m) {
        if (!isInt(mode, 1, 255)) return null;
        out.mode = mode;
      }
      if ("game" in m) {
        if (!isObject(game) || typeof game.code !== "string" || !isInt(game.n, 0) || !isInt(game.score, 0) || typeof game.over !== "boolean") return null;
        out.game = { code: game.code, n: game.n, score: game.score, over: game.over };
      }
      return out;
    }
  }
  return null;
}

export function encodeMessage(m: Message): string {
  switch (m.type) {
    case "start":
      return JSON.stringify({ m: MULTIPLAYER_VERSION, type: "start", code: m.code });
    default:
      return JSON.stringify({ m: MULTIPLAYER_VERSION, ...m });
  }
}

// ---- Nostr events (§3) ----

export interface NostrEvent {
  id: string;
  pubkey: string;
  created_at: number;
  kind: number;
  tags: string[][];
  content: string;
  sig: string;
}

const ESCAPES: Record<string, string> = { '"': '\\"', "\\": "\\\\", "\n": "\\n", "\r": "\\r", "\t": "\\t", "\b": "\\b", "\f": "\\f" };

/** JSON string per NIP-01: only " \ \n \r \t \b \f are escaped, everything else is verbatim. */
export function jsonString(s: string): string {
  return '"' + s.replace(/["\\\n\r\t\b\f]/g, (c) => ESCAPES[c]!) + '"';
}

export function serialise(pubkey: string, createdAt: number, kind: number, tags: string[][], content: string): string {
  const t = "[" + tags.map((tag) => "[" + tag.map(jsonString).join(",") + "]").join(",") + "]";
  return `[0,${jsonString(pubkey)},${createdAt},${kind},${t},${jsonString(content)}]`;
}

export function eventId(e: Omit<NostrEvent, "id" | "sig">): Uint8Array {
  return sha256(utf8.encode(serialise(e.pubkey, e.created_at, e.kind, e.tags, e.content)));
}

export function newSecretKey(): Uint8Array {
  return schnorr.utils.randomSecretKey();
}

export function publicKey(secret: Uint8Array): string {
  return bytesToHex(schnorr.getPublicKey(secret));
}

/** Signed event for a room. `aux`: BIP-340 auxiliary randomness (random unless given, for tests). */
export function createEvent(secret: Uint8Array, createdAt: number, roomId: string, content: string, aux?: Uint8Array): NostrEvent {
  const unsigned = { pubkey: publicKey(secret), created_at: createdAt, kind: KIND, tags: [["t", roomTag(roomId)]], content };
  const id = eventId(unsigned);
  return { ...unsigned, id: bytesToHex(id), sig: bytesToHex(schnorr.sign(id, secret, aux ?? crypto.getRandomValues(new Uint8Array(32)))) };
}

const HEX64 = /^[0-9a-f]{64}$/;

/** True if id and signature are valid (§3.3). */
export function verifyEvent(e: NostrEvent): boolean {
  try {
    if (!HEX64.test(e.id) || !HEX64.test(e.pubkey) || !/^[0-9a-f]{128}$/.test(e.sig)) return false;
    if (!Number.isInteger(e.created_at) || !Number.isInteger(e.kind) || typeof e.content !== "string") return false;
    if (!Array.isArray(e.tags) || !e.tags.every((t) => Array.isArray(t) && t.every((x) => typeof x === "string"))) return false;
    const id = eventId(e);
    if (bytesToHex(id) !== e.id) return false;
    return schnorr.verify(hexToBytes(e.sig), id, hexToBytes(e.pubkey));
  } catch {
    return false;
  }
}
