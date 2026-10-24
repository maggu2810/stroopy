# SPDX-License-Identifier: GPL-3.0-or-later
"""Reference implementation of docs/spec/share-code/v1.md."""
import re

VERSION = 1
ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
ALIASES = {"O": "0", "I": "1", "L": "1"}
PATTERN = re.compile(r"SYG([0-9]+)-([0-9A-Za-z]+(?:-[0-9A-Za-z]+)*)", re.IGNORECASE)


class Invalid(Exception):
    pass


class Unsupported(Exception):
    pass


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


def encode_record(rules: int, mode: int, seed: int, result=None) -> bytes:
    out = bytes([VERSION, rules, mode]) + seed.to_bytes(8, "big")
    if result is None:
        out += bytes([0])
    else:
        score, reached = (min(v, 0xFFFF) for v in result)
        out += bytes([1]) + score.to_bytes(2, "big") + reached.to_bytes(2, "big")
    return out + crc16(out).to_bytes(2, "big")


def base32(data: bytes) -> str:
    bits = len(data) * 8
    value = int.from_bytes(data, "big")
    chars = -(-bits // 5)
    value <<= chars * 5 - bits
    return "".join(ALPHABET[(value >> (5 * (chars - 1 - i))) & 31] for i in range(chars))


def unbase32(text: str, nbytes: int) -> bytes:
    value = 0
    for ch in text:
        value = (value << 5) | ALPHABET.index(ch)
    pad = len(text) * 5 - nbytes * 8
    if value & ((1 << pad) - 1):
        raise Invalid("non-zero padding bits")
    return (value >> pad).to_bytes(nbytes, "big")


def encode_text(record: bytes) -> str:
    body = base32(record[1:])
    return f"SYG{record[0]}-" + "-".join(body[i:i + 4] for i in range(0, len(body), 4))


def decode_text(text: str):
    m = PATTERN.search(text)
    if not m:
        raise Invalid("no code found")
    if int(m.group(1)) != VERSION:
        raise Unsupported(int(m.group(1)))
    body = "".join(ALIASES.get(c, c) for c in m.group(2).replace("-", "").upper())
    if any(c not in ALPHABET for c in body):
        raise Invalid("bad character")
    lengths = {21: 13, 28: 17}
    if len(body) not in lengths:
        raise Invalid("bad length")
    record = bytes([VERSION]) + unbase32(body, lengths[len(body)])
    if crc16(record[:-2]) != int.from_bytes(record[-2:], "big"):
        raise Invalid("bad checksum")
    flags = record[11]
    if flags & ~1 or (flags & 1) != (len(record) == 18):
        raise Invalid("bad flags")
    result = None
    if flags & 1:
        result = (int.from_bytes(record[12:14], "big"), int.from_bytes(record[14:16], "big"))
    return {"rules": record[1], "mode": record[2], "seed": int.from_bytes(record[3:11], "big"), "result": result}
