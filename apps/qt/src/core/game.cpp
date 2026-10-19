// SPDX-License-Identifier: GPL-3.0-or-later
#include "game.hpp"

#include <algorithm>

namespace stroopy {

using namespace rules::v1;

Game::Game(Mode mode, std::uint64_t seed) noexcept
    : m_mode{mode}, m_seed{seed}, m_words{mode, seed}, m_rounds{seed}, m_current{WordItem{}}
{
    advance();
}

int Game::n() const noexcept
{
    return std::visit([](const auto &c) { return c.n; }, m_current);
}

int Game::limitMs() const noexcept
{
    switch (m_mode) {
    case Mode::Chrono: return chronoTotalMs;
    case Mode::Tap: return roundLimitMs(n());
    case Mode::Classic:
    case Mode::Find: break;
    }
    return itemLimitMs(n());
}

bool Game::isCorrect(int choice) const noexcept
{
    if (const auto *round = std::get_if<TapRound>(&m_current))
        return choice == round->trueCard;
    const auto &item = std::get<WordItem>(m_current);
    if (m_mode == Mode::Find)
        return choice == item.word;
    return (choice == 1) == item.isTrue();
}

Game::Outcome Game::answer(int choice, int elapsedMs) noexcept
{
    if (m_over)
        return Outcome::GameOver;
    if (m_mode != Mode::Chrono && elapsedMs >= limitMs()) {
        timeout();
        return Outcome::GameOver;
    }
    const bool correct = isCorrect(choice);
    if (m_mode == Mode::Chrono) {
        m_score = correct ? m_score + 1 : std::max(0, m_score - 1);
        advance();
        return correct ? Outcome::Correct : Outcome::Wrong;
    }
    if (!correct) {
        m_over = true;
        return Outcome::GameOver;
    }
    ++m_score;
    advance();
    return Outcome::Correct;
}

void Game::timeout() noexcept
{
    m_over = true;
}

void Game::advance() noexcept
{
    if (m_mode == Mode::Tap)
        m_current = m_rounds.next();
    else
        m_current = m_words.next();
}

} // namespace stroopy
