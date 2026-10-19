// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamecontroller.hpp"

#include <QRandomGenerator>

using stroopy::Game;
using namespace stroopy::rules::v1;

namespace {
constexpr int countdownStepMs = 1000;
} // namespace

GameController::GameController(QObject *parent) : QObject{parent}
{
    m_countdownTimer.setInterval(countdownStepMs);
    connect(&m_countdownTimer, &QTimer::timeout, this, &GameController::tickCountdown);
    m_limitTimer.setSingleShot(true);
    m_limitTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_limitTimer, &QTimer::timeout, this, [this] {
        if (!m_game)
            return;
        m_game->timeout();
        finish();
    });
}

bool GameController::start(Mode mode, const QString &seedHex)
{
    std::uint64_t seed = 0;
    if (seedHex.isEmpty()) {
        seed = QRandomGenerator::system()->generate64();
    } else {
        bool ok = false;
        seed = seedHex.toULongLong(&ok, 16);
        if (!ok)
            return false;
    }
    abort();
    m_mode = mode;
    m_game.emplace(static_cast<Game::Mode>(mode), seed);
    emit gameChanged();
    emit challengeChanged();
    emit scoreChanged();
    m_countdown = 3;
    emit countdownChanged();
    setPhase(Phase::Countdown);
    m_countdownTimer.start();
    return true;
}

void GameController::restart()
{
    if (m_game)
        start(m_mode, seedHex());
}

void GameController::tickCountdown()
{
    if (--m_countdown < 0) {
        m_countdownTimer.stop();
        setPhase(Phase::Running);
        if (m_mode == Mode::Chrono)
            m_limitTimer.start(chronoTotalMs);
        showChallenge();
        return;
    }
    emit countdownChanged();
}

void GameController::showChallenge()
{
    emit challengeChanged();
    m_shown.start();
    if (m_mode != Mode::Chrono)
        m_limitTimer.start(m_game->limitMs());
}

void GameController::answer(int choice)
{
    if (m_phase != Phase::Running || !m_game)
        return;
    const auto elapsed = static_cast<int>(m_shown.elapsed());
    const auto outcome = m_game->answer(choice, elapsed);
    emit scoreChanged();
    emit answered(outcome == Game::Outcome::Correct);
    if (m_game->isOver())
        finish();
    else
        showChallenge();
}

void GameController::abort()
{
    m_countdownTimer.stop();
    m_limitTimer.stop();
    setPhase(Phase::Idle);
}

void GameController::finish()
{
    m_limitTimer.stop();
    setPhase(Phase::Over);
}

void GameController::setPhase(Phase phase)
{
    if (m_phase == phase)
        return;
    m_phase = phase;
    emit phaseChanged();
}

QString GameController::seedHex() const
{
    return m_game ? QStringLiteral("%1").arg(m_game->seed(), 16, 16, QLatin1Char('0')).toUpper() : QString{};
}

int GameController::n() const { return m_game ? m_game->n() : 0; }

int GameController::pool() const
{
    if (!m_game)
        return 4;
    return std::visit([](const auto &c) { return c.pool; }, m_game->current());
}

int GameController::ink() const
{
    const auto *item = m_game ? std::get_if<WordItem>(&m_game->current()) : nullptr;
    return item ? item->ink : 0;
}

int GameController::word() const
{
    const auto *item = m_game ? std::get_if<WordItem>(&m_game->current()) : nullptr;
    return item ? item->word : 0;
}

QList<int> GameController::cardInks() const
{
    const auto *round = m_game ? std::get_if<TapRound>(&m_game->current()) : nullptr;
    return round ? QList<int>{round->inks.begin(), round->inks.end()} : QList<int>{};
}

QList<int> GameController::cardWords() const
{
    const auto *round = m_game ? std::get_if<TapRound>(&m_game->current()) : nullptr;
    return round ? QList<int>{round->words.begin(), round->words.end()} : QList<int>{};
}

int GameController::limitMs() const { return m_game ? m_game->limitMs() : 0; }
int GameController::score() const { return m_game ? m_game->score() : 0; }
int GameController::reached() const { return m_game ? m_game->reached() : 0; }
