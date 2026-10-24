// SPDX-License-Identifier: GPL-3.0-or-later
// Share code v1 (docs/spec/share-code/v1.md): game descriptor <-> record bytes <-> text code.
import { base32, ByteWriter, crc16, getU16, getU64, grouped, invalid, normalise, unbase32, unsupported, type Decoded } from "./codec.ts";

export const SHARE_CODE_VERSION = 1;

export interface Descriptor {
  rules: number;
  mode: number;
  seed: bigint;
  result?: { score: number; reached: number };
}

export function encodeRecord(d: Descriptor): Uint8Array {
  const w = new ByteWriter().u8(SHARE_CODE_VERSION).u8(d.rules).u8(d.mode).u64(d.seed).u8(d.result ? 1 : 0);
  if (d.result) w.u16(d.result.score).u16(d.result.reached);
  return w.crc().result();
}

export function encodeText(d: Descriptor): string {
  return `SYG${SHARE_CODE_VERSION}-` + grouped(base32(encodeRecord(d).subarray(1)));
}

const PATTERN = /SYG([0-9]+)-([0-9A-Za-z]+(?:-[0-9A-Za-z]+)*)/i;

/** Finds and decodes the first share code in arbitrary text (spec §2.1). */
export function decodeText(text: string): Decoded<Descriptor> {
  const m = PATTERN.exec(text);
  if (!m) return invalid;
  const version = Number(m[1]);
  if (version !== SHARE_CODE_VERSION) return unsupported(version);
  let body = "";
  for (const raw of m[2]!.replaceAll("-", "")) {
    const c = normalise(raw);
    if (!c) return invalid;
    body += c;
  }
  const size = body.length === 21 ? 14 : body.length === 28 ? 18 : 0;
  if (!size) return invalid;
  const { bytes, rest } = unbase32(body);
  if (rest !== 0 || bytes.length !== size - 1) return invalid;
  const record = Uint8Array.from([SHARE_CODE_VERSION, ...bytes]);
  if (crc16(record.subarray(0, size - 2)) !== getU16(record, size - 2)) return invalid;
  const flags = record[11]!;
  if ((flags & ~1) !== 0 || (flags & 1) !== (size === 18 ? 1 : 0)) return invalid;
  const value: Descriptor = { rules: record[1]!, mode: record[2]!, seed: getU64(record, 3) };
  if (flags & 1) value.result = { score: getU16(record, 12), reached: getU16(record, 14) };
  return { ok: true, value };
}
