# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes docs/spec/test-vectors/protocol/v1/*.json from the reference implementation."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import protocol_v1 as pr  # noqa: E402
import share_code_v1 as sc  # noqa: E402

CODE = sc.encode_text(sc.encode_record(1, 1, 0x2A))


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
    if not isinstance(m, dict) or m.get("p") != 1:
        return None
    t = m.get("type")
    if t == "bye":
        return {"type": "bye"}
    if t == "start":
        if not isinstance(m.get("code"), str):
            return None
        try:
            d = sc.decode_text(m["code"])
        except (sc.Invalid, sc.Unsupported):
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
    common = {"protocol": 1, "generator": "tools/reference/gen_protocol_vectors.py"}

    # rooms
    ids = [0, 1, 0xFFFFFFFFFF, 0x3A5F07C2D1, 0x8000000001]
    encode = [{"value": f"0x{v:010x}", "id": pr.room_id(v), "code": pr.room_code(pr.room_id(v)),
               "tag": pr.room_tag(pr.room_id(v))} for v in ids]
    rid = encode[3]["id"]
    code = encode[3]["code"]
    valid = [
        {"input": code, "note": "canonical"},
        {"input": code.lower(), "note": "lower case"},
        {"input": code.replace("-", "", 2).replace("SYR1", "SYR1-"), "note": "without the middle dash"},
        {"input": f"Join my Stroopy room: {code} !", "note": "embedded in a message"},
        {"input": f"stroopy://join?r={code}", "note": "link"},
        {"input": f"  {rid[:4]}-{rid[4:]} ", "note": "bare id with dash, typed"},
        {"input": rid.lower(), "note": "bare id, lower case"},
        {"input": code.replace("0", "O").replace("1", "I").replace("SYRI", "SYR1"), "note": "aliases"},
    ]
    for v in valid:
        v["expected"] = pr.parse_room(v["input"])
        assert v["expected"] == rid, v
    errors = [
        {"input": "SYR1-ABCD-EFGU", "error": "invalid", "note": "U is not in the alphabet"},
        {"input": "ABCD-EFG", "error": "invalid", "note": "bare id too short"},
        {"input": "say ABCDEFGH please", "error": "invalid", "note": "bare id must be the whole input"},
        {"input": "SY1-040G-0000-0000-001A-03CT-T", "error": "invalid", "note": "a share code is not a room"},
        {"input": "SYR2-ABCD-EFGH", "error": "unsupported", "version": 2, "note": "unknown protocol version"},
    ]
    for d in errors:
        try:
            pr.parse_room(d["input"])
            raise SystemExit(f"expected failure: {d}")
        except pr.Unsupported:
            assert d["error"] == "unsupported"
        except pr.Invalid:
            assert d["error"] == "invalid"
    (out / "rooms.json").write_text(json.dumps({**common,
        "description": "Room ids (40-bit values) -> id, room code and Nostr room tag; parsing of room codes "
                       "and typed ids -> id, or an error ('invalid' or 'unsupported').",
        "encode": encode, "parse": valid, "errors": errors}, indent=1, ensure_ascii=False) + "\n")

    # events
    contents = [
        '{"p":1,"type":"bye"}',
        json.dumps({"p": 1, "type": "start", "code": CODE}, separators=(",", ":")),
        json.dumps({"p": 1, "type": "status", "name": "Ann", "host": True, "rules": [1], "mode": 1,
                    "game": {"code": CODE, "n": 12, "score": 11, "over": False}}, separators=(",", ":")),
        'quote " backslash \\ newline \n cr \r tab \t bs \b ff \f ctrl \u0001 del \u007f ä € 🎨 </script>',
    ]
    secrets = [(1).to_bytes(32, "big"), bytes.fromhex("c90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b14e5c9")]
    events = []
    for i, content in enumerate(contents):
        secret = secrets[i % 2]
        ev, ser = pr.event(secret, 1792483200 + i, rid, content)
        assert pr.verify(bytes.fromhex(ev["id"]), bytes.fromhex(ev["pubkey"]), bytes.fromhex(ev["sig"]))
        events.append({"secretKey": secret.hex(), "createdAt": ev["created_at"], "roomId": rid,
                       "content": content, "serialised": ser, "event": ev})
    bad_sig = dict(events[0]["event"])
    bad_sig["sig"] = bad_sig["sig"][:-1] + ("0" if bad_sig["sig"][-1] != "0" else "1")
    bad_id = dict(events[0]["event"], content='{"p":1,"type":"bye" }')
    (out / "events.json").write_text(json.dumps({**common,
        "description": "Nostr events (NIP-01 serialisation, sha256 id, BIP-340 signature with 32 zero bytes of "
                       "auxiliary randomness) for a room. 'reject': received events that must be dropped.",
        "kind": pr.KIND, "auxRand": "00" * 32, "sign": events,
        "reject": [{"event": bad_sig, "note": "signature does not verify"},
                   {"event": bad_id, "note": "id does not match the content"}]}, indent=1, ensure_ascii=False) + "\n")

    # messages
    msgs = [
        (contents[0], "bye"),
        (contents[1], "start"),
        (contents[2], "status with game"),
        ('{"p":1,"type":"status","name":"Bob","host":false,"rules":[1,2],"extra":{"x":1}}', "unknown field ignored"),
        ('{"p":1,"type":"status","name":"  \\u0007Clara\\n  ","host":false,"rules":[1]}', "name cleaned"),
        ('{"p":1,"type":"status","name":"' + "Ü" * 30 + '","host":false,"rules":[1]}', "name truncated to 24"),
        ('{"p":1,"type":"status","name":"\\n","host":false,"rules":[1]}', "empty name becomes ?"),
        ('{"p":1,"type":"status","name":"Dan","host":false,"rules":[1],"game":{"code":"' + CODE
         + '","n":48,"score":47,"over":true}}', "game over"),
        ('{"p":2,"type":"status","name":"Eve","host":false,"rules":[1]}', "other protocol version"),
        ('{"p":1,"type":"hello"}', "unknown type"),
        ('{"p":1,"type":"status","name":"Eve","host":"yes","rules":[1]}', "host not a bool"),
        ('{"p":1,"type":"status","name":"Eve","host":false,"rules":[]}', "empty rules"),
        ('{"p":1,"type":"status","name":"Eve","host":false,"rules":[1],"game":{"code":"x","n":-1,"score":0,'
         '"over":false}}', "negative n"),
        ('{"p":1,"type":"start","code":"SY1-040G-0000-0000-001A-0402-Y01G-J960"}', "start code with a result"),
        ('{"p":1,"type":"start","code":"nonsense"}', "start code invalid"),
        ('[1,2,3]', "not an object"),
        ('{"p":1,"type":', "not JSON"),
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
