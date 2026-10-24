# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes docs/spec/test-vectors/backup/v1/*.json from the reference implementation."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import backup_v1 as bk  # noqa: E402
import share_code_v1 as sc  # noqa: E402


def hx(v: int) -> str:
    return f"0x{v:016x}"


def entry_json(e):
    return {**e, "seed": hx(e["seed"])}


def settings_json(s):
    return None if s is None else {str(k): v for k, v in sorted(s.items())}


def reseal(record: bytearray) -> str:
    record[-2:] = sc.crc16(bytes(record[:-2])).to_bytes(2, "big")
    return bk.encode_text(bytes(record))


def main(out: Path):
    out.mkdir(parents=True, exist_ok=True)
    common = {"backup": 1, "generator": "tools/reference/gen_backup_vectors.py"}
    e1 = {"mode": 1, "rules": 1, "seed": 0x2A, "score": 47, "reached": 48, "time": 1792483200, "challenge": False}
    e2 = {"mode": 4, "rules": 1, "seed": 0xFFFFFFFFFFFFFFFF, "score": 12, "reached": 13, "time": 1792569600,
          "challenge": True}
    e3 = {"mode": 9, "rules": 7, "seed": 0x0123456789ABCDEF, "score": 3, "reached": 4, "time": 0, "challenge": False}
    full = {bk.KEY_LANGUAGE: "de", bk.KEY_COLOUR_BLIND: True, bk.KEY_VIBRATION: False, bk.KEY_NAME: "Änne 🎨",
            bk.KEY_RELAYS: "wss://relay.damus.io\nwss://nos.lol"}
    cases = [
        ([], None, "empty, no settings"),
        ([], {}, "empty settings section"),
        ([e1], {bk.KEY_LANGUAGE: "", bk.KEY_COLOUR_BLIND: False, bk.KEY_VIBRATION: True}, "defaults"),
        ([e1, e2, e3], full, "all settings; unknown mode and rules are kept"),
        ([{**e1, "score": 70000, "reached": 100000}], None, "score/reached clamped to 65535"),
    ]
    encode = []
    for entries, settings, note in cases:
        rec = bk.encode_record(entries, settings)
        encode.append({"entries": [entry_json(e) for e in entries], "settings": settings_json(settings),
                       "record": rec.hex(), "text": bk.encode_text(rec), "note": note})
    # 40 entries: a realistic full backup
    many = [{"mode": 1 + i % 4, "rules": 1, "seed": (i * 0x9E3779B97F4A7C15) & (2**64 - 1), "score": 100 - i,
             "reached": 101 - i, "time": 1792483200 + i * 3600, "challenge": i % 3 == 0} for i in range(40)]
    rec = bk.encode_record(many, full)
    encode.append({"entries": [entry_json(e) for e in many], "settings": settings_json(full),
                   "record": rec.hex(), "text": bk.encode_text(rec), "note": "full backup, 40 entries"})
    (out / "encode.json").write_text(json.dumps({**common,
        "description": "Entries + settings (keys as strings; null = no settings section) -> record bytes (hex) "
                       "and text code. Decoding 'text' must give them back (with clamped values).",
        "cases": encode}, indent=1, ensure_ascii=False) + "\n")

    base = encode[3]["text"]
    body = base[5:]
    wrapped = "SYB1-" + "\n".join(body[i:i + 30] for i in range(0, len(body), 30))
    valid = [
        {"input": base, "note": "canonical"},
        {"input": base.lower(), "note": "lower case"},
        {"input": "SYB1-" + body.replace("-", ""), "note": "without group separators"},
        {"input": wrapped, "note": "wrapped over several lines"},
        {"input": f"My Stroopy backup:\n{base}\nThank you!", "note": "embedded; text after the code is ignored"},
        {"input": f"{base}-ABCD", "note": "extra characters after the code are ignored"},
        {"input": "SYB1-" + body.replace("0", "O").replace("1", "l"), "note": "O/L aliases"},
    ]
    unknown_key = bytearray(bk.encode_record([e1], {bk.KEY_VIBRATION: True}))
    # append an unknown key 42 with 3 bytes of value in front of the CRC; bump the item count
    count_pos = 4 + 19
    unknown_key[count_pos] += 1
    unknown_key[-2:-2] = bytes([42, 0, 3, 1, 2, 3])
    valid.append({"input": reseal(unknown_key), "note": "unknown settings key is skipped"})
    for v in valid:
        r = bk.decode_text(v["input"])
        v["expected"] = {"entries": [entry_json(e) for e in r["entries"]], "settings": settings_json(r["settings"])}

    def broken(mutate):
        rec = bytearray(bk.encode_record([e1], {bk.KEY_COLOUR_BLIND: True, bk.KEY_NAME: "Ann"}))
        mutate(rec)
        return reseal(rec)

    s_pos = 4 + 19  # settings section start
    pad_body = sc.base32(bk.encode_record([], {})[1:])  # 6 bytes -> 10 chars, 2 padding bits
    errors = [
        {"input": base[:20] + ("1" if base[20] != "1" else "2") + base[21:], "note": "checksum mismatch"},
        {"input": base[:-3], "note": "too short"},
        {"input": "SYB1-" + pad_body[:-1] + sc.ALPHABET[sc.ALPHABET.index(pad_body[-1]) | 1],
         "note": "non-zero padding bits"},
        {"input": broken(lambda r: r.__setitem__(1, 3)), "note": "unknown flag bit"},
        {"input": broken(lambda r: r.__setitem__(4 + 18, 2)), "note": "unknown entry flag bit"},
        {"input": broken(lambda r: r.__setitem__(s_pos + 4, 2)), "note": "boolean value 2"},
        {"input": broken(lambda r: r.__setitem__(s_pos + 1 + 4, 2)), "note": "repeated key"},
        {"input": broken(lambda r: r.__setitem__(s_pos + 1 + 4, 200)), "note": "reserved key"},
        {"input": broken(lambda r: r.__setitem__(s_pos + 1 + 4 + 3, 0xFF)), "note": "invalid UTF-8"},
        {"input": "nothing here", "note": "no backup"},
        {"input": base.replace("SYB1-", "SYG1-"), "note": "share-code prefix is not a backup"},
    ]
    for d in errors:
        d["error"] = "invalid"
    errors.append({"input": base.replace("SYB1-", "SYB2-"), "error": "unsupported", "version": 2,
                   "note": "unknown backup version"})
    for d in errors:
        try:
            bk.decode_text(d["input"])
            raise SystemExit(f"expected failure: {d}")
        except bk.Unsupported:
            assert d["error"] == "unsupported", d
        except bk.Invalid:
            assert d["error"] == "invalid", d
    (out / "decode.json").write_text(json.dumps({**common,
        "description": "Text input -> entries and settings (keys as strings, unknown keys dropped; "
                       "null = no settings section), or an error ('invalid' or 'unsupported').",
        "valid": valid, "errors": errors}, indent=1, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
