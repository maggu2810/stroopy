# SPDX-License-Identifier: GPL-3.0-or-later
"""Reference implementation of docs/spec/multiplayer/v1.md: invitations and Nostr events.

BIP-340 Schnorr signatures are written from the BIP's specification in plain
Python. Slow and not constant-time: for generating test vectors only."""
import hashlib
import re

from game_v1 import ALIASES, ALPHABET, base32, crc16

VERSION = 1
KIND = 25713
PREFIX = re.compile(r"SYM([0-9]+)-", re.IGNORECASE)
TAG_NOSTR = 1
MAX_RELAYS = 8


class Invalid(Exception):
    pass


class Unsupported(Exception):
    pass


class NoTransport(Exception):
    pass


# --- invitations ---------------------------------------------------------

def room_id(value: int) -> str:
    """40-bit integer -> 8 Crockford characters."""
    return "".join(ALPHABET[(value >> (35 - 5 * i)) & 31] for i in range(8))


def encode_record(room: int, entries) -> bytes:
    """entries: list of (tag, value bytes)."""
    out = bytearray([VERSION]) + room.to_bytes(5, "big") + bytes([len(entries)])
    for tag, value in entries:
        out += bytes([tag, len(value)]) + value
    return bytes(out) + crc16(bytes(out)).to_bytes(2, "big")


def encode_text(record: bytes) -> str:
    body = base32(record[1:])
    return f"SYM{record[0]}-" + "-".join(body[i:i + 4] for i in range(0, len(body), 4))


def invitation(room: int, relays) -> str:
    return encode_text(encode_record(room, [(TAG_NOSTR, r.encode("utf-8")) for r in relays]))


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


def _relay(value: bytes) -> str:
    try:
        url = value.decode("utf-8")
    except UnicodeDecodeError:
        raise Invalid("bad utf-8")
    for scheme in ("wss://", "ws://"):
        if url.startswith(scheme) and len(url) > len(scheme):
            break
    else:
        raise Invalid("bad relay scheme")
    if any(ord(c) <= 0x20 or ord(c) == 0x7F for c in url):
        raise Invalid("bad relay character")
    return url


def decode_text(text: str):
    """Returns {"room": id, "relays": [...]}. Raises Invalid, Unsupported or NoTransport."""
    m = PREFIX.search(text)
    if not m:
        raise Invalid("no invitation found")
    if int(m.group(1)) != VERSION:
        raise Unsupported(int(m.group(1)))
    chars = _collect(text[m.end():])
    data = bytes([VERSION]) + _bytes(chars)
    if len(data) < 7:
        raise Invalid("too short")
    count = data[6]
    if count == 0:
        raise Invalid("no entries")
    relays, pos = [], 7
    for _ in range(count):
        if len(data) < pos + 2:
            raise Invalid("too short")
        tag, length = data[pos], data[pos + 1]
        value = data[pos + 2:pos + 2 + length]
        if len(value) != length:
            raise Invalid("too short")
        pos += 2 + length
        if tag >= 128:
            raise Invalid("reserved tag")
        if tag == TAG_NOSTR:
            url = _relay(value)
            if url in relays:
                raise Invalid("repeated relay")
            relays.append(url)
            if len(relays) > MAX_RELAYS:
                raise Invalid("too many relays")
    if len(data) < pos + 2:
        raise Invalid("too short")
    length = pos + 2
    needed = -(-(length - 1) * 8 // 5)
    pad = needed * 5 - (length - 1) * 8
    if ALPHABET.index(chars[needed - 1]) & ((1 << pad) - 1):
        raise Invalid("non-zero padding bits")
    if crc16(data[:pos]) != int.from_bytes(data[pos:pos + 2], "big"):
        raise Invalid("bad checksum")
    if not relays:
        raise NoTransport()
    return {"room": room_id(int.from_bytes(data[1:6], "big")), "relays": relays}


def room_tag(rid: str) -> str:
    return f"stroopy-m{VERSION}-{rid.lower()}"


# --- NIP-01 serialisation --------------------------------------------------

_ESCAPES = {'"': '\\"', "\\": "\\\\", "\n": "\\n", "\r": "\\r", "\t": "\\t", "\b": "\\b", "\f": "\\f"}


def json_string(s: str) -> str:
    return '"' + "".join(_ESCAPES.get(c, c) for c in s) + '"'


def serialise(pubkey: str, created_at: int, kind: int, tags, content: str) -> str:
    tags_json = "[" + ",".join("[" + ",".join(json_string(t) for t in tag) + "]" for tag in tags) + "]"
    return f"[0,{json_string(pubkey)},{created_at},{kind},{tags_json},{json_string(content)}]"


# --- BIP-340 -------------------------------------------------------------

P = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
G = (0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798,
     0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8)


def _add(a, b):
    if a is None:
        return b
    if b is None:
        return a
    if a[0] == b[0] and a[1] != b[1]:
        return None
    if a == b:
        lam = 3 * a[0] * a[0] * pow(2 * a[1], P - 2, P) % P
    else:
        lam = (b[1] - a[1]) * pow(b[0] - a[0], P - 2, P) % P
    x = (lam * lam - a[0] - b[0]) % P
    return x, (lam * (a[0] - x) - a[1]) % P


def _mul(point, k):
    result = None
    for i in range(256):
        if (k >> i) & 1:
            result = _add(result, point)
        point = _add(point, point)
    return result


def _tagged(tag: str, data: bytes) -> bytes:
    t = hashlib.sha256(tag.encode()).digest()
    return hashlib.sha256(t + t + data).digest()


def _b(x: int) -> bytes:
    return x.to_bytes(32, "big")


def pubkey(secret: bytes) -> bytes:
    return _b(_mul(G, int.from_bytes(secret, "big"))[0])


def sign(msg: bytes, secret: bytes, aux: bytes) -> bytes:
    d0 = int.from_bytes(secret, "big")
    if not 1 <= d0 < N:
        raise ValueError("bad secret key")
    pt = _mul(G, d0)
    d = d0 if pt[1] % 2 == 0 else N - d0
    t = _b(d ^ int.from_bytes(_tagged("BIP0340/aux", aux), "big"))
    k0 = int.from_bytes(_tagged("BIP0340/nonce", t + _b(pt[0]) + msg), "big") % N
    r = _mul(G, k0)
    k = k0 if r[1] % 2 == 0 else N - k0
    e = int.from_bytes(_tagged("BIP0340/challenge", _b(r[0]) + _b(pt[0]) + msg), "big") % N
    return _b(r[0]) + _b((k + e * d) % N)


def verify(msg: bytes, pub: bytes, sig: bytes) -> bool:
    x = int.from_bytes(pub, "big")
    y = pow((pow(x, 3, P) + 7) % P, (P + 1) // 4, P)
    if (y * y - x ** 3 - 7) % P:
        return False
    pt = (x, y if y % 2 == 0 else P - y)
    r, s = int.from_bytes(sig[:32], "big"), int.from_bytes(sig[32:], "big")
    if r >= P or s >= N:
        return False
    e = int.from_bytes(_tagged("BIP0340/challenge", sig[:32] + pub + msg), "big") % N
    rr = _add(_mul(G, s), _mul(pt, N - e))
    return rr is not None and rr[1] % 2 == 0 and rr[0] == r


def event(secret: bytes, created_at: int, rid: str, content: str, aux: bytes = bytes(32)):
    pub = pubkey(secret).hex()
    tags = [["t", room_tag(rid)]]
    ser = serialise(pub, created_at, KIND, tags, content)
    eid = hashlib.sha256(ser.encode("utf-8")).digest()
    return {"id": eid.hex(), "pubkey": pub, "created_at": created_at, "kind": KIND, "tags": tags,
            "content": content, "sig": sign(eid, secret, aux).hex()}, ser
