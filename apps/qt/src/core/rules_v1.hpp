// SPDX-License-Identifier: GPL-3.0-or-later
// Rules v1 (docs/spec/rules/v1.md): PRNG, pool, item generation and time limits.
#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace stroopy::rules::v1 {

inline constexpr int version = 1;

enum class Mode : std::uint8_t { Classic = 1, Chrono = 2, Find = 3, Tap = 4 };

inline constexpr int chronoTotalMs = 30000;
inline constexpr int maxPoolSize = 8;

class SplitMix64 {
public:
    explicit constexpr SplitMix64(std::uint64_t seed) noexcept : m_state{seed} {}

    constexpr std::uint64_t next() noexcept
    {
        m_state += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z = m_state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Uniform value in [0, k), 1 <= k <= 2^32 (spec §2.1).
    constexpr std::uint64_t below(std::uint64_t k) noexcept
    {
        const std::uint64_t threshold = (0 - k) % k;
        for (;;) {
            if (const std::uint64_t x = next(); x >= threshold)
                return x % k;
        }
    }

    // Uniform value in [0, k) without e, k >= 2 (spec §2.2).
    constexpr std::uint64_t belowExcept(std::uint64_t k, std::uint64_t e) noexcept
    {
        const std::uint64_t r = below(k - 1);
        return r >= e ? r + 1 : r;
    }

    constexpr bool coin() noexcept { return below(2) == 1; }

private:
    std::uint64_t m_state;
};

int poolSize(int n, Mode mode) noexcept;
int itemLimitMs(int n) noexcept;
int roundLimitMs(int n) noexcept;

// One item of the word modes (classic, chrono, find).
struct WordItem {
    int n;
    int pool;
    int ink;
    int word;
    [[nodiscard]] constexpr bool isTrue() const noexcept { return ink == word; }
};

class WordItemGenerator {
public:
    WordItemGenerator(Mode mode, std::uint64_t seed) noexcept;
    WordItem next() noexcept;

private:
    Mode m_mode;
    SplitMix64 m_rng;
    int m_n = 0;
    int m_prevInk = 0;
};

// One round of the tap mode.
struct TapRound {
    int n;
    int pool;
    std::array<int, 4> inks;
    std::array<int, 4> words;
    int trueCard;
};

class TapRoundGenerator {
public:
    explicit TapRoundGenerator(std::uint64_t seed) noexcept;
    TapRound next() noexcept;

private:
    SplitMix64 m_rng;
    int m_n = 0;
};

std::optional<Mode> modeFromId(int id) noexcept;

} // namespace stroopy::rules::v1
