// SPDX-License-Identifier: GPL-3.0-or-later
// Backup code v1 (docs/spec/backup/v1.md): highscores and shared settings <-> record bytes <-> text code.
import { ALPHABET, base32, ByteWriter, crc16, getU16, getU32, getU64, grouped, invalid, normalise, unbase32, unsupported, type Decoded } from "./codec.ts";

export const BACKUP_VERSION = 1;
export const MAX_NAME_BYTES = 64;
const ENTRY_SIZE = 19;

export interface Entry {
  mode: number;
  rules: number;
  seed: bigint;
  score: number;
  reached: number;
  /** Unix seconds, UTC */
  time: number;
  challenge: boolean;
}

/** Shared settings; undefined = not in the backup (or skipped when decoding). */
export interface Settings {
  language?: string; // "" = follow the system
  colourBlind?: boolean;
  vibration?: boolean;
  name?: string;
  relays?: string; // "\n"-separated, "" = default relays
}

export interface Backup {
  entries: Entry[];
  settings?: Settings; // undefined = no settings section
}

const Key = { Language: 1, ColourBlind: 2, Vibration: 3, Name: 4, Relays: 5 } as const;

const utf8 = new TextEncoder();
const strictUtf8 = new TextDecoder("utf-8", { fatal: true });

function truncateUtf8(s: string, maxBytes: number): Uint8Array {
  let out = utf8.encode(s);
  if (out.length <= maxBytes) return out;
  let cut = maxBytes;
  while (cut > 0 && (out[cut]! & 0xc0) === 0x80) --cut; // don't split a character
  return out.subarray(0, cut);
}

export function encodeRecord(b: Backup): Uint8Array {
  const w = new ByteWriter().u8(BACKUP_VERSION).u8(b.settings ? 1 : 0).u16(b.entries.length);
  for (const e of b.entries) {
    w.u8(e.mode).u8(e.rules).u64(e.seed).u16(e.score).u16(e.reached).u32(e.time).u8(e.challenge ? 1 : 0);
  }
  const s = b.settings;
  if (s) {
    const items: [number, Uint8Array][] = [];
    if (s.language !== undefined) items.push([Key.Language, utf8.encode(s.language)]);
    if (s.colourBlind !== undefined) items.push([Key.ColourBlind, Uint8Array.of(s.colourBlind ? 1 : 0)]);
    if (s.vibration !== undefined) items.push([Key.Vibration, Uint8Array.of(s.vibration ? 1 : 0)]);
    if (s.name !== undefined) items.push([Key.Name, truncateUtf8(s.name, MAX_NAME_BYTES)]);
    if (s.relays !== undefined) items.push([Key.Relays, utf8.encode(s.relays)]);
    w.u8(items.length);
    for (const [key, value] of items) w.u8(key).u16(value.length).raw(value);
  }
  return w.crc().result();
}

export function encodeText(b: Backup): string {
  return `SYB${BACKUP_VERSION}-` + grouped(base32(encodeRecord(b).subarray(1)));
}

class TooShort extends Error {}
class Bad extends Error {}

const PREFIX = /SYB([0-9]+)-/i;

/** Finds and decodes the first backup code in arbitrary text (spec §2.1). */
export function decodeText(text: string): Decoded<Backup> {
  const m = PREFIX.exec(text);
  if (!m) return invalid;
  const version = Number(m[1]);
  if (version !== BACKUP_VERSION) return unsupported(version);

  let chars = "";
  for (const raw of text.slice(m.index + m[0].length)) {
    if (raw === "-" || /\s/.test(raw)) continue;
    const c = normalise(raw);
    if (!c) break;
    chars += c;
  }
  const data = Uint8Array.from([BACKUP_VERSION, ...unbase32(chars).bytes]);
  let pos = 1;
  const need = (n: number) => {
    if (pos + n > data.length) throw new TooShort();
  };
  const u8 = () => (need(1), data[pos++]!);
  const u16 = () => (need(2), (pos += 2), getU16(data, pos - 2));

  try {
    const flags = u8();
    const count = u16();
    if (flags & ~1) throw new Bad();
    const backup: Backup = { entries: [] };
    for (let i = 0; i < count; ++i) {
      need(ENTRY_SIZE);
      const o = pos;
      if (data[o + 18]! & ~1) throw new Bad();
      backup.entries.push({
        mode: data[o]!,
        rules: data[o + 1]!,
        seed: getU64(data, o + 2),
        score: getU16(data, o + 10),
        reached: getU16(data, o + 12),
        time: getU32(data, o + 14),
        challenge: (data[o + 18]! & 1) === 1,
      });
      pos += ENTRY_SIZE;
    }
    if (flags & 1) {
      const s: Settings = {};
      const seen = new Set<number>();
      const items = u8();
      for (let i = 0; i < items; ++i) {
        const key = u8();
        const len = u16();
        need(len);
        const value = data.subarray(pos, pos + len);
        pos += len;
        if (seen.has(key) || key >= 128) throw new Bad();
        seen.add(key);
        const str = () => {
          try {
            return strictUtf8.decode(value);
          } catch {
            throw new Bad();
          }
        };
        switch (key) {
          case Key.ColourBlind:
          case Key.Vibration:
            if (len !== 1 || value[0]! > 1) throw new Bad();
            if (key === Key.ColourBlind) s.colourBlind = value[0] === 1;
            else s.vibration = value[0] === 1;
            break;
          case Key.Language:
            s.language = str();
            break;
          case Key.Relays:
            s.relays = str();
            break;
          case Key.Name: {
            const name = str();
            // eslint-disable-next-line no-control-regex
            if (len > MAX_NAME_BYTES || /[\u0000-\u001f\u007f]/.test(name)) throw new Bad();
            s.name = name;
            break;
          }
          default:
            break; // unknown key: skipped
        }
      }
      backup.settings = s;
    }
    const crcPos = pos;
    u16();
    // Exactly the characters needed for the record belong to the code (§2.1 step 4).
    const length = crcPos + 2;
    const needed = Math.ceil(((length - 1) * 8) / 5);
    if (needed > chars.length) return invalid;
    const pad = needed * 5 - (length - 1) * 8;
    if ((ALPHABET.indexOf(chars[needed - 1]!) & ((1 << pad) - 1)) !== 0) return invalid;
    if (crc16(data.subarray(0, crcPos)) !== getU16(data, crcPos)) return invalid;
    return { ok: true, value: backup };
  } catch (e) {
    if (e instanceof TooShort || e instanceof Bad) return invalid;
    throw e;
  }
}
