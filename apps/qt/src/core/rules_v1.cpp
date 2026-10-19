// SPDX-License-Identifier: GPL-3.0-or-later
#include "rules_v1.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <utility>

namespace stroopy::rules::v1 {

namespace {
constexpr std::array wordThresholds{20, 30, 40, 50};
constexpr std::array tapThresholds{6, 11, 16, 21};
} // namespace

int poolSize(int n, Mode mode) noexcept
{
    const auto &thresholds = mode == Mode::Tap ? tapThresholds : wordThresholds;
    return 4 + static_cast<int>(std::ranges::count_if(thresholds, [n](int t) { return t <= n; }));
}

int itemLimitMs(int n) noexcept
{
    const int base = n == 1 ? 6000
                   : n < 10 ? 4000
                   : n < 20 ? 3000
                   : n < 30 ? 2500
                   : n < 40 ? 2000
                            : 1600;
    return std::max(250, base * (300 - n) / 300);
}

int roundLimitMs(int n) noexcept
{
    return std::max(250, 2000 - 10 * (n - 1));
}

WordItemGenerator::WordItemGenerator(Mode mode, std::uint64_t seed) noexcept
    : m_mode{mode}, m_rng{seed}
{
}

WordItem WordItemGenerator::next() noexcept
{
    const int n = ++m_n;
    const int p = poolSize(n, m_mode);
    const auto up = static_cast<std::uint64_t>(p);
    const auto ink = n == 1 ? m_rng.below(up) : m_rng.belowExcept(up, static_cast<std::uint64_t>(m_prevInk));
    const auto word = m_mode == Mode::Find || !m_rng.coin() ? m_rng.belowExcept(up, ink) : ink;
    m_prevInk = static_cast<int>(ink);
    return {.n = n, .pool = p, .ink = static_cast<int>(ink), .word = static_cast<int>(word)};
}

TapRoundGenerator::TapRoundGenerator(std::uint64_t seed) noexcept : m_rng{seed} {}

TapRound TapRoundGenerator::next() noexcept
{
    const int n = ++m_n;
    const int p = poolSize(n, Mode::Tap);
    const auto up = static_cast<std::uint64_t>(p);
    std::array<int, maxPoolSize> a{};
    std::iota(a.begin(), a.end(), 0);
    for (std::size_t i = 0; i < 4; ++i) {
        const auto j = i + static_cast<std::size_t>(m_rng.below(up - i));
        std::swap(a[i], a[j]);
    }
    TapRound round{.n = n, .pool = p, .inks = {a[0], a[1], a[2], a[3]}, .words = {}, .trueCard = 0};
    round.trueCard = static_cast<int>(m_rng.below(4));
    for (std::size_t c = 0; c < 4; ++c) {
        const auto ink = static_cast<std::uint64_t>(round.inks[c]);
        round.words[c] = std::cmp_equal(c, round.trueCard)
                           ? round.inks[c]
                           : static_cast<int>(m_rng.belowExcept(up, ink));
    }
    return round;
}

std::optional<Mode> modeFromId(int id) noexcept
{
    if (id >= 1 && id <= 4)
        return static_cast<Mode>(id);
    return std::nullopt;
}

} // namespace stroopy::rules::v1
