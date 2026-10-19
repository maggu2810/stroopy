// SPDX-License-Identifier: GPL-3.0-or-later
// Game session per docs/spec/rules/v1.md §6: answers, scoring and end of game.
// Time is supplied by the caller, so the logic stays deterministic and testable.
#pragma once

#include "rules_v1.hpp"

#include <cstdint>
#include <variant>

namespace stroopy {

class Game {
public:
    using Mode = rules::v1::Mode;
    using Challenge = std::variant<rules::v1::WordItem, rules::v1::TapRound>;

    enum class Outcome : std::uint8_t { Correct, Wrong, GameOver };

    Game(Mode mode, std::uint64_t seed) noexcept;

    [[nodiscard]] Mode mode() const noexcept { return m_mode; }
    [[nodiscard]] std::uint64_t seed() const noexcept { return m_seed; }
    [[nodiscard]] const Challenge &current() const noexcept { return m_current; }
    [[nodiscard]] int n() const noexcept;
    // Limit for the current item/round in ms; for chrono the total game limit.
    [[nodiscard]] int limitMs() const noexcept;

    // choice: classic/chrono 1 = TRUE, 0 = FALSE; find: colour slot; tap: card position.
    // elapsedMs: since the current item was shown (classic/find/tap), ignored for chrono.
    Outcome answer(int choice, int elapsedMs) noexcept;
    // The current item's limit (or chrono's total limit) has been reached.
    void timeout() noexcept;

    [[nodiscard]] bool isOver() const noexcept { return m_over; }
    [[nodiscard]] int score() const noexcept { return m_score; }
    // Number of the item/round that failed or was unanswered at the end (spec §6).
    [[nodiscard]] int reached() const noexcept { return n(); }

private:
    [[nodiscard]] bool isCorrect(int choice) const noexcept;
    void advance() noexcept;

    Mode m_mode;
    std::uint64_t m_seed;
    rules::v1::WordItemGenerator m_words;
    rules::v1::TapRoundGenerator m_rounds;
    Challenge m_current;
    int m_score = 0;
    bool m_over = false;
};

} // namespace stroopy
