// SPDX-License-Identifier: GPL-3.0-or-later
// Protocol v1 (docs/spec/protocol/v1.md): room codes, messages and Nostr events.
import { schnorr } from "@noble/curves/secp256k1.js";
import { sha256 } from "@noble/hashes/sha2.js";
import { bytesToHex, hexToBytes } from "@noble/hashes/utils.js";
import { ALPHABET, invalid, normalise, unsupported, type Decoded } from "../core/codec.ts";
import { decodeText } from "../core/share-code-v1.ts";

export const PROTOCOL_VERSION = 1;
export const KIND = 25713;

// ---- rooms (§1) ----

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

export function roomCode(id: string): string {
  return `SYR${PROTOCOL_VERSION}-${id.slice(0, 4)}-${id.slice(4)}`;
}

export function roomTag(id: string): string {
  return `stroopy-p${PROTOCOL_VERSION}-${id.toLowerCase()}`;
}

const ROOM_PATTERN = /SYR([0-9]+)-([0-9A-Za-z]{4})-?([0-9A-Za-z]{4})/i;
const BARE_PATTERN = /^([0-9A-Za-z]{4})-?([0-9A-Za-z]{4})$/;

function normaliseId(chars: string): string | null {
  let out = "";
  for (const c of chars) {
    const n = normalise(c);
    if (!n) return null;
    out += n;
  }
  return out;
}

/** Room code in arbitrary text, or a bare id typed by a user -> room id. */
export function parseRoom(text: string): Decoded<string> {
  const m = ROOM_PATTERN.exec(text);
  if (m) {
    const version = Number(m[1]);
    if (version !== PROTOCOL_VERSION) return unsupported(version);
    const id = normaliseId(m[2]! + m[3]!);
    return id ? { ok: true, value: id } : invalid;
  }
  const b = BARE_PATTERN.exec(text.trim());
  const id = b && normaliseId(b[1]! + b[2]!);
  return id ? { ok: true, value: id } : invalid;
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
  if (!isObject(m) || m.p !== PROTOCOL_VERSION) return null;
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
      return JSON.stringify({ p: PROTOCOL_VERSION, type: "start", code: m.code });
    default:
      return JSON.stringify({ p: PROTOCOL_VERSION, ...m });
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

const utf8 = new TextEncoder();

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
