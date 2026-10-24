# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes docs/spec/test-vectors/share-code/v1/*.json from the reference implementation."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import share_code_v1 as sc  # noqa: E402


def hx(v: int) -> str:
    return f"0x{v:016x}"


def main(out: Path):
    out.mkdir(parents=True, exist_ok=True)
    common = {"shareCode": 1, "generator": "tools/reference/gen_share_code_vectors.py"}
    assert sc.crc16(b"123456789") == 0x29B1

    descriptors = [
        (1, 1, 0x0, None), (1, 1, 0x2A, None), (1, 2, 0x0123456789ABCDEF, None),
        (1, 3, 0xFFFFFFFFFFFFFFFF, None), (1, 4, 0x8000000000000001, None),
        (1, 1, 0x2A, (47, 48)), (1, 2, 0xDEADBEEFCAFEBABE, (0, 31)),
        (1, 4, 0x0123456789ABCDEF, (65535, 65535)), (7, 9, 0x1122334455667788, (3, 4)),
    ]
    encode = []
    for rules, mode, seed, result in descriptors:
        rec = sc.encode_record(rules, mode, seed, result)
        entry = {"rules": rules, "mode": mode, "seed": hx(seed)}
        if result:
            entry["result"] = {"score": result[0], "reached": result[1]}
        entry["record"] = rec.hex()
        entry["text"] = sc.encode_text(rec)
        encode.append(entry)
    clamp = sc.encode_record(1, 1, 5, (70000, 100000))
    encode.append({"rules": 1, "mode": 1, "seed": hx(5), "result": {"score": 70000, "reached": 100000},
                   "record": clamp.hex(), "text": sc.encode_text(clamp), "note": "score/reached clamped to 65535"})
    (out / "encode.json").write_text(json.dumps({**common,
        "description": "Descriptor (+ optional result) -> record bytes (hex) and text code. "
                       "Decoding 'text' must give the descriptor back (with clamped values).",
        "crc16Check": {"input": "123456789", "crc": "0x29b1"},
        "cases": encode}, indent=1) + "\n")

    base = encode[5]["text"]          # with result
    plain = encode[1]["text"]
    body = base[len("SYG1-"):]
    decode = [
        {"input": base, "note": "canonical"},
        {"input": base.lower(), "note": "lower case"},
        {"input": "SYG1-" + body.replace("-", ""), "note": "without group separators"},
        {"input": f"I scored 47 in Stroopy Classic: {base} — beat me!", "note": "embedded in a message"},
        {"input": f"stroopy://play?c={plain}", "note": "link"},
        {"input": "SYG1-" + body.replace("0", "O").replace("1", "I"), "note": "O/I aliases"},
        {"input": "SYG1-" + body.replace("1", "l"), "note": "lower-case L alias"},
    ]
    for d in decode:
        r = sc.decode_text(d["input"])
        d["expected"] = {"rules": r["rules"], "mode": r["mode"], "seed": hx(r["seed"])}
        if r["result"]:
            d["expected"]["result"] = {"score": r["result"][0], "reached": r["result"][1]}

    def flip_last(text):  # change the last character to break the checksum
        last = text[-1]
        return text[:-1] + ("1" if last != "1" else "2")

    bad_flags = bytearray(sc.encode_record(1, 1, 42))
    bad_flags[11] = 2
    bad_flags[-2:] = sc.crc16(bytes(bad_flags[:-2])).to_bytes(2, "big")
    pad_body = sc.base32(sc.encode_record(1, 1, 42)[1:])
    nonzero_pad = pad_body[:-1] + sc.ALPHABET[sc.ALPHABET.index(pad_body[-1]) | 1]
    invalid = [
        {"input": flip_last(base), "error": "invalid", "note": "checksum mismatch"},
        {"input": base[:-1], "error": "invalid", "note": "too short"},
        {"input": base + "0", "error": "invalid", "note": "too long"},
        {"input": "SYG1-" + body[:5] + "U" + body[6:], "error": "invalid", "note": "U is not in the alphabet"},
        {"input": sc.encode_text(bytes(bad_flags)), "error": "invalid", "note": "unknown flag bit"},
        {"input": "SYG1-" + nonzero_pad, "error": "invalid", "note": "non-zero padding bits"},
        {"input": "hello world", "error": "invalid", "note": "no code"},
        {"input": "SY1-" + body, "error": "invalid", "note": "prefix without the type letter G"},
        {"input": "SYG2-" + body, "error": "unsupported", "version": 2, "note": "unknown share-code version"},
    ]
    for d in invalid:
        try:
            sc.decode_text(d["input"])
            raise SystemExit(f"expected failure: {d}")
        except sc.Unsupported:
            assert d["error"] == "unsupported", d
        except sc.Invalid:
            assert d["error"] == "invalid", d
    (out / "decode.json").write_text(json.dumps({**common,
        "description": "Text input -> decoded descriptor, or an error ('invalid' or 'unsupported').",
        "valid": decode, "errors": invalid}, indent=1) + "\n")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
