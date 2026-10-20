# SPDX-License-Identifier: GPL-3.0-or-later
"""Reference implementation of docs/spec/backup/v1.md."""
import re

from share_code_v1 import ALIASES, ALPHABET, base32, crc16

VERSION = 1
PREFIX = re.compile(r"SYB([0-9]+)-", re.IGNORECASE)
ENTRY_SIZE = 19
KEY_LANGUAGE, KEY_COLOUR_BLIND, KEY_VIBRATION, KEY_NAME, KEY_RELAYS = 1, 2, 3, 4, 5


class Invalid(Exception):
    pass


class Unsupported(Exception):
    pass


def u16(v):
    return min(v, 0xFFFF).to_bytes(2, "big")


def encode_record(entries, settings=None) -> bytes:
    """entries: dicts mode, rules, seed, score, reached, time, challenge.
    settings: dict key -> value (str for 1/4/5, bool for 2/3) or None."""
    out = bytearray([VERSION, 1 if settings is not None else 0]) + len(entries).to_bytes(2, "big")
    for e in entries:
        out += bytes([e["mode"], e["rules"]]) + e["seed"].to_bytes(8, "big")
        out += u16(e["score"]) + u16(e["reached"]) + e["time"].to_bytes(4, "big")
        out += bytes([1 if e["challenge"] else 0])
    if settings is not None:
        out.append(len(settings))
        for key in sorted(settings):
            v = settings[key]
            value = bytes([1 if v else 0]) if isinstance(v, bool) else v.encode("utf-8")
            out += bytes([key]) + len(value).to_bytes(2, "big") + value
    return bytes(out) + crc16(bytes(out)).to_bytes(2, "big")


def encode_text(record: bytes) -> str:
    body = base32(record[1:])
    return f"SYB{record[0]}-" + "-".join(body[i:i + 4] for i in range(0, len(body), 4))


def _collect(text: str) -> str:
    chars = []
    for c in text:
        if c == "-" or c.isspace():
            continue
        c = ALIASES.get(c.upper(), c.upper())
        if c not in ALPHABET or not c.isascii():
            break
        chars.append(c)
    return "".join(chars)


def _bytes(chars: str) -> bytes:
    value, bits, out = 0, 0, bytearray()
    for c in chars:
        value = (value << 5) | ALPHABET.index(c)
        bits += 5
        if bits >= 8:
            bits -= 8
            out.append((value >> bits) & 0xFF)
    return bytes(out)


def _parse_settings(data: bytes, pos: int):
    """Returns (settings dict, end offset). Raises Invalid / IndexError."""
    count = data[pos]
    pos += 1
    settings = {}
    for _ in range(count):
        key = data[pos]
        length = int.from_bytes(data[pos + 1:pos + 3], "big")
        value = data[pos + 3:pos + 3 + length]
        if len(value) != length:
            raise IndexError
        pos += 3 + length
        if key in settings:
            raise Invalid("repeated key")
        if key >= 128:
            raise Invalid("reserved key")
        if key in (KEY_COLOUR_BLIND, KEY_VIBRATION):
            if length != 1 or value[0] > 1:
                raise Invalid("bad boolean")
            settings[key] = value[0] == 1
        elif key in (KEY_LANGUAGE, KEY_NAME, KEY_RELAYS):
            try:
                s = value.decode("utf-8")
            except UnicodeDecodeError:
                raise Invalid("bad utf-8")
            if key == KEY_NAME and (length > 64 or any(ord(c) < 0x20 or ord(c) == 0x7F for c in s)):
                raise Invalid("bad name")
            settings[key] = s
        else:
            settings[key] = None  # unknown: skipped
    return {k: v for k, v in settings.items() if v is not None}, pos


def decode_text(text: str):
    m = PREFIX.search(text)
    if not m:
        raise Invalid("no backup found")
    if int(m.group(1)) != VERSION:
        raise Unsupported(int(m.group(1)))
    chars = _collect(text[m.end():])
    data = bytes([VERSION]) + _bytes(chars)
    try:
        flags = data[1]
        if flags & ~1:
            raise Invalid("bad flags")
        count = int.from_bytes(data[2:4], "big")
        if len(data) < 4:
            raise IndexError
        entries, pos = [], 4
        for _ in range(count):
            e = data[pos:pos + ENTRY_SIZE]
            if len(e) != ENTRY_SIZE:
                raise IndexError
            if e[18] & ~1:
                raise Invalid("bad entry flags")
            entries.append({"mode": e[0], "rules": e[1], "seed": int.from_bytes(e[2:10], "big"),
                            "score": int.from_bytes(e[10:12], "big"), "reached": int.from_bytes(e[12:14], "big"),
                            "time": int.from_bytes(e[14:18], "big"), "challenge": bool(e[18] & 1)})
            pos += ENTRY_SIZE
        settings = None
        if flags & 1:
            settings, pos = _parse_settings(data, pos)
        if len(data) < pos + 2:
            raise IndexError
    except IndexError:
        raise Invalid("too short")
    length = pos + 2
    needed = -(-(length - 1) * 8 // 5)
    if needed > len(chars):
        raise Invalid("too short")
    pad = needed * 5 - (length - 1) * 8
    if ALPHABET.index(chars[needed - 1]) & ((1 << pad) - 1):
        raise Invalid("non-zero padding bits")
    if crc16(data[:pos]) != int.from_bytes(data[pos:pos + 2], "big"):
        raise Invalid("bad checksum")
    return {"entries": entries, "settings": settings}
