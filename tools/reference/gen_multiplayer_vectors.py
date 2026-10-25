# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes docs/spec/test-vectors/multiplayer/v1/*.json from the reference implementation."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import game_v1 as gc  # noqa: E402
import multiplayer_v1 as mp  # noqa: E402

CODE = gc.encode_text(gc.encode_record(1, 1, 0x2A))


def clean_name(name: str) -> str:
    s = "".join(c for c in name if not (ord(c) < 0x20 or ord(c) == 0x7F)).strip()[:24]
    return s or "?"


def is_int(v, lo, hi=None):
    return isinstance(v, int) and not isinstance(v, bool) and v >= lo and (hi is None or v <= hi)


def parse_message(content: str):
    """Returns the normalised message, or None if it is ignored."""
    try:
        m = json.loads(content)
    except ValueError:
        return None
    if not isinstance(m, dict) or m.get("m") != 1:
        return None
    t = m.get("type")
    if t == "bye":
        return {"type": "bye"}
    if t == "start":
        if not isinstance(m.get("code"), str):
            return None
        try:
            d = gc.decode_text(m["code"])
        except (gc.Invalid, gc.Unsupported):
            return None
        if d["result"] is not None:
            return None
        return {"type": "start", "rules": d["rules"], "mode": d["mode"], "seed": f"0x{d['seed']:016x}"}
    if t == "status":
        if not isinstance(m.get("name"), str) or not isinstance(m.get("host"), bool):
            return None
        rules = m.get("rules")
        if not isinstance(rules, list) or not rules or not all(is_int(r, 1, 255) for r in rules):
            return None
        out = {"type": "status", "name": clean_name(m["name"]), "host": m["host"], "rules": rules}
        if "mode" in m:
            if not is_int(m["mode"], 1, 255):
                return None
            out["mode"] = m["mode"]
        if "game" in m:
            g = m["game"]
            if not (isinstance(g, dict) and isinstance(g.get("code"), str) and is_int(g.get("n"), 0)
                    and is_int(g.get("score"), 0) and isinstance(g.get("over"), bool)):
                return None
            out["game"] = {k: g[k] for k in ("code", "n", "score", "over")}
        return out
    return None


def main(out: Path):
    out.mkdir(parents=True, exist_ok=True)
    common = {"multiplayer": 1, "generator": "tools/reference/gen_multiplayer_vectors.py"}

    # invitations
    damus, nos, primal = "wss://relay.damus.io", "wss://nos.lol", "wss://relay.primal.net"
    room = 0x3A4D3BEA3C
    rid = mp.room_id(room)
    cases = [
        (0x0000000000, [damus], "lowest room id"),
        (room, [damus], "one relay"),
        (room, [damus, nos, primal], "three relays (default list of 2026-10-25)"),
        (0xFFFFFFFFFF, ["ws://localhost:7777"], "highest room id, local relay without TLS"),
        (0x8000000001, [f"wss://relay{i}.example.org" for i in range(1, 9)], "eight relays, the maximum"),
        (room, ["wss://relä.example/ñ"], "non-ASCII URL"),
    ]
    encode = []
    for value, relays, note in cases:
        rec = mp.encode_record(value, [(mp.TAG_NOSTR, r.encode("utf-8")) for r in relays])
        encode.append({"room": f"0x{value:010x}", "roomId": mp.room_id(value), "relays": relays,
                       "record": rec.hex(), "text": mp.encode_text(rec), "tag": mp.room_tag(mp.room_id(value)),
                       "note": note})
        assert mp.decode_text(encode[-1]["text"]) == {"room": mp.room_id(value), "relays": relays}
    assert encode[1]["roomId"] == "796KQTHW"

    one, three = encode[1]["text"], encode[2]["text"]
    body = three[len("SYM1-"):]
    wrapped = "SYM1-" + body[:40] + "\n" + body[40:80] + "\r\n " + body[80:]
    unknown = mp.encode_text(mp.encode_record(room, [(2, bytes([192, 168, 1, 5, 0xB8, 0x09])),
                                                     (mp.TAG_NOSTR, damus.encode()), (127, b"")]))
    valid = [
        {"input": three, "note": "canonical"},
        {"input": three.lower(), "note": "lower case"},
        {"input": "SYM1-" + body.replace("-", ""), "note": "without group separators"},
        {"input": wrapped, "note": "wrapped over lines"},
        {"input": f"Play Stroopy with me: {one} - see you!", "note": "embedded in a message"},
        {"input": f"stroopy://join?c={one}", "note": "link"},
        {"input": f"https://example.org/stroopy/?j={one}&x=1", "note": "web app link with trailing parameter"},
        {"input": "SYM1-" + one[5:].replace("0", "O").replace("1", "I"), "note": "O/I aliases"},
        {"input": unknown, "note": "unknown transport tags 2 and 127 are skipped"},
    ]
    for v in valid:
        v["expected"] = mp.decode_text(v["input"])

    def seal(room_value, entries, count=None, tamper=None):
        rec = bytearray(mp.encode_record(room_value, entries))
        if count is not None:
            rec[6] = count
        if tamper:
            tamper(rec)
        rec[-2:] = gc.crc16(bytes(rec[:-2])).to_bytes(2, "big")
        return mp.encode_text(bytes(rec))

    def flip_last(text):
        return text[:-1] + ("1" if text[-1] != "1" else "2")

    pad_body = gc.base32(mp.encode_record(room, [(mp.TAG_NOSTR, b"wss://a")])[1:])  # 16 bytes, 2 padding bits
    nostr = [(mp.TAG_NOSTR, r.encode()) for r in (damus, nos)]
    errors = [
        {"input": flip_last(one), "error": "invalid", "note": "checksum mismatch"},
        {"input": one[:-3], "error": "invalid", "note": "too short"},
        {"input": "SYM1-" + pad_body[:-1] + gc.ALPHABET[gc.ALPHABET.index(pad_body[-1]) | 1], "error": "invalid",
         "note": "non-zero padding bits"},
        {"input": seal(room, []), "error": "invalid", "note": "no entries (K = 0)"},
        {"input": seal(room, nostr + [(128, b"")]), "error": "invalid", "note": "reserved tag 128"},
        {"input": seal(room, [(mp.TAG_NOSTR, b"https://nos.lol")]), "error": "invalid", "note": "relay scheme not ws(s)"},
        {"input": seal(room, [(mp.TAG_NOSTR, b"WSS://nos.lol")]), "error": "invalid", "note": "upper-case scheme"},
        {"input": seal(room, [(mp.TAG_NOSTR, b"wss://")]), "error": "invalid", "note": "nothing after the scheme"},
        {"input": seal(room, [(mp.TAG_NOSTR, b"wss://nos .lol")]), "error": "invalid", "note": "space in relay URL"},
        {"input": seal(room, [(mp.TAG_NOSTR, b"wss://\xff")]), "error": "invalid", "note": "relay not UTF-8"},
        {"input": seal(room, nostr + [nostr[0]]), "error": "invalid", "note": "repeated relay"},
        {"input": seal(room, [(mp.TAG_NOSTR, f"wss://r{i}.example".encode()) for i in range(9)]), "error": "invalid",
         "note": "nine relays"},
        {"input": seal(room, nostr, count=3), "error": "invalid", "note": "K larger than the entries present"},
        {"input": "SYM1-" + body[:8], "error": "invalid", "note": "room id only"},
        {"input": "SYG1-040G-0000-0000-001A-03CT-T", "error": "invalid", "note": "a game code is not an invitation"},
        {"input": "796K-QTHW", "error": "invalid", "note": "a bare room id is not an invitation"},
        {"input": "SYM2-" + body, "error": "unsupported", "version": 2, "note": "unknown multiplayer version"},
        {"input": seal(room, [(2, bytes([192, 168, 1, 5, 0xB8, 0x09]))]), "error": "noTransport",
         "note": "only an unknown transport"},
    ]
    for d in errors:
        try:
            mp.decode_text(d["input"])
            raise SystemExit(f"expected failure: {d}")
        except mp.Unsupported:
            assert d["error"] == "unsupported", d
        except mp.NoTransport:
            assert d["error"] == "noTransport", d
        except mp.Invalid:
            assert d["error"] == "invalid", d
    (out / "invitations.json").write_text(json.dumps({**common,
        "description": "Invitations: room (40-bit value) and relays -> record bytes (hex), text code and Nostr "
                       "room tag; parsing of text input -> room id and relays, or an error ('invalid', "
                       "'unsupported' or 'noTransport').",
        "encode": encode, "parse": valid, "errors": errors}, indent=1, ensure_ascii=False) + "\n")

    # events
    contents = [
        '{"m":1,"type":"bye"}',
        json.dumps({"m": 1, "type": "start", "code": CODE}, separators=(",", ":")),
        json.dumps({"m": 1, "type": "status", "name": "Ann", "host": True, "rules": [1], "mode": 1,
                    "game": {"code": CODE, "n": 12, "score": 11, "over": False}}, separators=(",", ":")),
        'quote " backslash \\ newline \n cr \r tab \t bs \b ff \f ctrl \u0001 del \u007f ä € 🎨 </script>',
    ]
    secrets = [(1).to_bytes(32, "big"), bytes.fromhex("c90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b14e5c9")]
    events = []
    for i, content in enumerate(contents):
        secret = secrets[i % 2]
        ev, ser = mp.event(secret, 1792483200 + i, rid, content)
        assert mp.verify(bytes.fromhex(ev["id"]), bytes.fromhex(ev["pubkey"]), bytes.fromhex(ev["sig"]))
        events.append({"secretKey": secret.hex(), "createdAt": ev["created_at"], "roomId": rid,
                       "content": content, "serialised": ser, "event": ev})
    bad_sig = dict(events[0]["event"])
    bad_sig["sig"] = bad_sig["sig"][:-1] + ("0" if bad_sig["sig"][-1] != "0" else "1")
    bad_id = dict(events[0]["event"], content='{"m":1,"type":"bye" }')
    (out / "events.json").write_text(json.dumps({**common,
        "description": "Nostr events (NIP-01 serialisation, sha256 id, BIP-340 signature with 32 zero bytes of "
                       "auxiliary randomness) for a room. 'reject': received events that must be dropped.",
        "kind": mp.KIND, "auxRand": "00" * 32, "sign": events,
        "reject": [{"event": bad_sig, "note": "signature does not verify"},
                   {"event": bad_id, "note": "id does not match the content"}]}, indent=1, ensure_ascii=False) + "\n")

    # messages
    msgs = [
        (contents[0], "bye"),
        (contents[1], "start"),
        (contents[2], "status with game"),
        ('{"m":1,"type":"status","name":"Bob","host":false,"rules":[1,2],"extra":{"x":1}}', "unknown field ignored"),
        ('{"m":1,"type":"status","name":"  \\u0007Clara\\n  ","host":false,"rules":[1]}', "name cleaned"),
        ('{"m":1,"type":"status","name":"' + "Ü" * 30 + '","host":false,"rules":[1]}', "name truncated to 24"),
        ('{"m":1,"type":"status","name":"\\n","host":false,"rules":[1]}', "empty name becomes ?"),
        ('{"m":1,"type":"status","name":"Dan","host":false,"rules":[1],"game":{"code":"' + CODE
         + '","n":48,"score":47,"over":true}}', "game over"),
        ('{"m":2,"type":"status","name":"Eve","host":false,"rules":[1]}', "other multiplayer version"),
        ('{"m":1,"type":"hello"}', "unknown type"),
        ('{"m":1,"type":"status","name":"Eve","host":"yes","rules":[1]}', "host not a bool"),
        ('{"m":1,"type":"status","name":"Eve","host":false,"rules":[]}', "empty rules"),
        ('{"m":1,"type":"status","name":"Eve","host":false,"rules":[1],"game":{"code":"x","n":-1,"score":0,'
         '"over":false}}', "negative n"),
        ('{"m":1,"type":"start","code":"SYG1-040G-0000-0000-001A-0402-Y01G-J960"}', "start code with a result"),
        ('{"m":1,"type":"start","code":"nonsense"}', "start code invalid"),
        ('[1,2,3]', "not an object"),
        ('{"m":1,"type":', "not JSON"),
    ]
    cases = []
    for content, note in msgs:
        parsed = parse_message(content)
        cases.append({"content": content, "note": note, "expected": parsed})
    (out / "messages.json").write_text(json.dumps({**common,
        "description": "Event content -> normalised message, or null when the message must be ignored. "
                       "For 'start' the decoded descriptor (rules, mode, seed) is given.",
        "cases": cases}, indent=1, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
