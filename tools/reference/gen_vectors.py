# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes docs/spec/test-vectors/rules/v1/*.json from the reference implementation."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import rules_v1 as r  # noqa: E402

SEEDS = [0x0, 0x1, 0x2A, 0x12D687, 0x0123456789ABCDEF, 0xFFFFFFFFFFFFFFFF]


def hx(v: int) -> str:
    return f"0x{v:016x}"


def write(out: Path, name: str, doc: dict):
    out.mkdir(parents=True, exist_ok=True)
    (out / name).write_text(json.dumps(doc, indent=1) + "\n")


def main(out: Path):
    common = {"rules": 1, "generator": "tools/reference/gen_vectors.py"}

    cases = []
    for s in SEEDS:
        g = r.SplitMix64(s)
        cases.append({"seed": hx(s), "next": [hx(g.next()) for _ in range(8)]})
    write(out, "splitmix64.json", {**common, "description": "Raw next() outputs (u64 as hex strings).", "cases": cases})

    cases = []
    for s in SEEDS:
        for k in (1, 2, 3, 4, 7, 8, 1000, 1 << 32):
            g = r.SplitMix64(s)
            cases.append({"seed": hx(s), "k": k, "values": [g.below(k) for _ in range(16)]})
    for s in SEEDS[:3]:
        for k, e in ((2, 0), (2, 1), (4, 0), (4, 3), (8, 5)):
            g = r.SplitMix64(s)
            cases.append({"seed": hx(s), "k": k, "except": e, "values": [g.below_except(k, e) for _ in range(16)]})
    write(out, "bounded.json", {**common, "description": "Successive below(k) or belowExcept(k, except) calls on a fresh PRNG.", "cases": cases})

    ns = list(range(1, 61)) + [99, 100, 149, 150, 175, 176, 177, 250, 298, 299, 300, 301, 1000]
    write(out, "timing.json", {**common,
        "description": "Per-item (classic/find) and per-round (tap) limits in ms; chrono total.",
        "chronoTotalMs": r.CHRONO_TOTAL_MS,
        "itemLimit": [{"n": n, "ms": r.item_limit(n)} for n in ns],
        "roundLimit": [{"n": n, "ms": r.round_limit(n)} for n in ns]})

    for mode in ("classic", "chrono", "find"):
        cases = []
        for s in SEEDS:
            items = [{"n": n, "pool": p, "ink": ink, "word": word} for n, p, ink, word in r.word_items(mode, s, 80)]
            cases.append({"seed": hx(s), "items": items})
        write(out, f"items-{mode}.json", {**common, "mode": mode, "modeId": r.MODES[mode],
            "description": "First 80 items per seed.", "cases": cases})

    cases = []
    for s in SEEDS:
        rounds = [{"n": n, "pool": p, "inks": inks, "words": words, "trueCard": tc}
                  for n, p, inks, words, tc in r.tap_rounds(s, 40)]
        cases.append({"seed": hx(s), "rounds": rounds})
    write(out, "rounds-tap.json", {**common, "mode": "tap", "modeId": 4,
        "description": "First 40 rounds per seed.", "cases": cases})


if __name__ == "__main__":
    main(Path(sys.argv[1]))
