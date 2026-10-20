# SPDX-License-Identifier: GPL-3.0-or-later
"""Reference implementation of docs/spec/protocol/v1.md: room codes and Nostr events.

BIP-340 Schnorr signatures are written from the BIP's specification in plain
Python. Slow and not constant-time: for generating test vectors only."""
import hashlib
import re

from share_code_v1 import ALIASES, ALPHABET

VERSION = 1
KIND = 25713
ROOM_PATTERN = re.compile(r"SYR([0-9]+)-([0-9A-Za-z]{4})-?([0-9A-Za-z]{4})", re.IGNORECASE)
BARE_PATTERN = re.compile(r"^([0-9A-Za-z]{4})-?([0-9A-Za-z]{4})$")


class Invalid(Exception):
    pass


class Unsupported(Exception):
    pass


# --- rooms ---------------------------------------------------------------

def room_id(value: int) -> str:
    """40-bit integer -> 8 Crockford characters."""
    return "".join(ALPHABET[(value >> (35 - 5 * i)) & 31] for i in range(8))


def room_code(rid: str) -> str:
    return f"SYR{VERSION}-{rid[:4]}-{rid[4:]}"


def _normalise(chars: str) -> str:
    out = "".join(ALIASES.get(c, c) for c in chars.upper())
    if any(c not in ALPHABET for c in out):
        raise Invalid("bad character")
    return out


def parse_room(text: str) -> str:
    m = ROOM_PATTERN.search(text)
    if m:
        if int(m.group(1)) != VERSION:
            raise Unsupported(int(m.group(1)))
        return _normalise(m.group(2) + m.group(3))
    m = BARE_PATTERN.match(text.strip())
    if m:
        return _normalise(m.group(1) + m.group(2))
    raise Invalid("no room code")


def room_tag(rid: str) -> str:
    return f"stroopy-p{VERSION}-{rid.lower()}"


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
