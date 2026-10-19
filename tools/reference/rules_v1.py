# SPDX-License-Identifier: GPL-3.0-or-later
"""Reference implementation of docs/spec/rules/v1.md (used to generate test vectors)."""

MASK = (1 << 64) - 1

MODES = {"classic": 1, "chrono": 2, "find": 3, "tap": 4}
THRESHOLDS = {
    "classic": (20, 30, 40, 50),
    "chrono": (20, 30, 40, 50),
    "find": (20, 30, 40, 50),
    "tap": (6, 11, 16, 21),
}
CHRONO_TOTAL_MS = 30000


class SplitMix64:
    def __init__(self, seed: int):
        self.state = seed & MASK

    def next(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & MASK
        z = self.state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK
        return z ^ (z >> 31)

    def below(self, k: int) -> int:
        assert 1 <= k <= 1 << 32
        threshold = ((1 << 64) - k) % k
        while True:
            x = self.next()
            if x >= threshold:
                return x % k

    def below_except(self, k: int, e: int) -> int:
        assert k >= 2 and 0 <= e < k
        r = self.below(k - 1)
        return r + 1 if r >= e else r

    def coin(self) -> bool:
        return self.below(2) == 1


def pool_size(n: int, mode: str) -> int:
    return 4 + sum(1 for t in THRESHOLDS[mode] if t <= n)


def item_limit(n: int) -> int:
    if n == 1:
        base = 6000
    elif n < 10:
        base = 4000
    elif n < 20:
        base = 3000
    elif n < 30:
        base = 2500
    elif n < 40:
        base = 2000
    else:
        base = 1600
    return max(250, base * (300 - n) // 300) if n < 300 else 250


def round_limit(n: int) -> int:
    return max(250, 2000 - 10 * (n - 1))


def word_items(mode: str, seed: int, count: int):
    assert mode in ("classic", "chrono", "find")
    rng = SplitMix64(seed)
    prev_ink = None
    for n in range(1, count + 1):
        p = pool_size(n, mode)
        ink = rng.below(p) if n == 1 else rng.below_except(p, prev_ink)
        if mode == "find":
            word = rng.below_except(p, ink)
        else:
            word = ink if rng.coin() else rng.below_except(p, ink)
        prev_ink = ink
        yield n, p, ink, word


def tap_rounds(seed: int, count: int):
    rng = SplitMix64(seed)
    for n in range(1, count + 1):
        p = pool_size(n, "tap")
        a = list(range(p))
        for i in range(4):
            j = i + rng.below(p - i)
            a[i], a[j] = a[j], a[i]
        inks = a[:4]
        true_card = rng.below(4)
        words = [inks[c] if c == true_card else rng.below_except(p, inks[c]) for c in range(4)]
        yield n, p, inks, words, true_card
