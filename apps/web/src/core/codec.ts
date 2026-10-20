// SPDX-License-Identifier: GPL-3.0-or-later
// Building blocks shared by the text codes: Crockford Base32 and CRC-16/CCITT-FALSE
// (docs/spec/share-code/v1.md §1-2).

export const ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

export type DecodeError = { kind: "invalid" } | { kind: "unsupported"; version: number };
export type Decoded<T> = { ok: true; value: T } | ({ ok: false } & DecodeError);

export const invalid = { ok: false, kind: "invalid" } as const;

export function unsupported(version: number) {
  return { ok: false, kind: "unsupported", version } as const;
}

export function crc16(data: Uint8Array): number {
  let crc = 0xffff;
  for (const b of data) {
    crc ^= b << 8;
    for (let i = 0; i < 8; ++i) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
  }
  return crc;
}

/** Big-endian bit string in 5-bit groups, the last one padded with zero bits. */
export function base32(data: Uint8Array): string {
  let out = "";
  let buffer = 0;
  let bits = 0;
  for (const b of data) {
    buffer = ((buffer << 8) | b) & 0xffff;
    bits += 8;
    while (bits >= 5) {
      bits -= 5;
      out += ALPHABET[(buffer >> bits) & 31];
    }
  }
  if (bits > 0) out += ALPHABET[(buffer << (5 - bits)) & 31];
  return out;
}

/** All whole bytes held by `chars` (alphabet characters only), plus the leftover bits. */
export function unbase32(chars: string): { bytes: Uint8Array; rest: number; restBits: number } {
  const out: number[] = [];
  let buffer = 0;
  let bits = 0;
  for (const c of chars) {
    buffer = ((buffer << 5) | ALPHABET.indexOf(c)) & 0xffff;
    bits += 5;
    if (bits >= 8) {
      bits -= 8;
      out.push((buffer >> bits) & 0xff);
    }
  }
  return { bytes: Uint8Array.from(out), rest: buffer & ((1 << bits) - 1), restBits: bits };
}

/** Upper case plus the aliases O->0, I->1, L->1; "" if the result is not in the alphabet. */
export function normalise(c: string): string {
  const u = c.toUpperCase();
  const m = u === "O" ? "0" : u === "I" || u === "L" ? "1" : u;
  return m.length === 1 && ALPHABET.includes(m) ? m : "";
}

export function grouped(text: string): string {
  return text.match(/.{1,4}/g)?.join("-") ?? "";
}

export class ByteWriter {
  private bytes: number[] = [];

  u8(v: number): this {
    this.bytes.push(v & 0xff);
    return this;
  }
  u16(v: number): this {
    const c = Math.min(Math.max(Math.trunc(v), 0), 0xffff);
    return this.u8(c >> 8).u8(c);
  }
  u32(v: number): this {
    const c = Math.min(Math.max(Math.trunc(v), 0), 0xffffffff);
    return this.u16(Math.floor(c / 0x10000)).u16(c % 0x10000);
  }
  u64(v: bigint): this {
    for (let shift = 56n; shift >= 0n; shift -= 8n) this.u8(Number((v >> shift) & 0xffn));
    return this;
  }
  raw(data: Uint8Array): this {
    this.bytes.push(...data);
    return this;
  }
  /** Appends the CRC-16 of everything written so far. */
  crc(): this {
    return this.u16(crc16(this.result()));
  }
  result(): Uint8Array {
    return Uint8Array.from(this.bytes);
  }
}

export function getU16(d: Uint8Array, o: number): number {
  return (d[o]! << 8) | d[o + 1]!;
}

export function getU32(d: Uint8Array, o: number): number {
  return getU16(d, o) * 0x10000 + getU16(d, o + 2);
}

export function getU64(d: Uint8Array, o: number): bigint {
  let v = 0n;
  for (let i = 0; i < 8; ++i) v = (v << 8n) | BigInt(d[o + i]!);
  return v;
}

export function seedToHex(seed: bigint): string {
  return seed.toString(16).toUpperCase().padStart(16, "0");
}
