// SPDX-License-Identifier: GPL-3.0-or-later
// Bridges the pure C++ game core to QML: countdown, timers and current challenge.
#pragma once

#include "game.hpp"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

#include <optional>

class GameController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Phase phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(int countdown READ countdown NOTIFY countdownChanged)
    Q_PROPERTY(Mode mode READ mode NOTIFY gameChanged)
    Q_PROPERTY(QString seedHex READ seedHex NOTIFY gameChanged)
    Q_PROPERTY(int n READ n NOTIFY challengeChanged)
    Q_PROPERTY(int pool READ pool NOTIFY challengeChanged)
    Q_PROPERTY(int ink READ ink NOTIFY challengeChanged)
    Q_PROPERTY(int word READ word NOTIFY challengeChanged)
    Q_PROPERTY(QList<int> cardInks READ cardInks NOTIFY challengeChanged)
    Q_PROPERTY(QList<int> cardWords READ cardWords NOTIFY challengeChanged)
    Q_PROPERTY(int limitMs READ limitMs NOTIFY challengeChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int reached READ reached NOTIFY phaseChanged)
    Q_PROPERTY(int rules READ rules CONSTANT)
    // Result of the player who shared the code this game was started from; -1 if none.
    Q_PROPERTY(int challengeScore READ challengeScore NOTIFY gameChanged)
    Q_PROPERTY(int challengeReached READ challengeReached NOTIFY gameChanged)
    Q_PROPERTY(bool isChallenge READ isChallenge NOTIFY gameChanged)

public:
    enum class Phase { Idle, Countdown, Running, Over };
    Q_ENUM(Phase)
    // Values match rules v1 mode ids.
    enum class Mode { Classic = 1, Chrono = 2, Find = 3, Tap = 4 };
    Q_ENUM(Mode)

    explicit GameController(QObject *parent = nullptr);

    // seedHex: 16 hex digits; empty for a random seed. Returns false on a bad seed.
    Q_INVOKABLE bool start(GameController::Mode mode, const QString &seedHex = {});
    // Starts a game from a shared code's descriptor, keeping the sharer's result to beat.
    Q_INVOKABLE bool startChallenge(GameController::Mode mode, const QString &seedHex, int score, int reached);
    Q_INVOKABLE void restart();
    // classic/chrono: 1 = TRUE, 0 = FALSE; find: colour slot; tap: card position.
    Q_INVOKABLE void answer(int choice);
    Q_INVOKABLE void abort();

    Phase phase() const { return m_phase; }
    int countdown() const { return m_countdown; }
    Mode mode() const { return m_mode; }
    QString seedHex() const;
    int n() const;
    int pool() const;
    int ink() const;
    int word() const;
    QList<int> cardInks() const;
    QList<int> cardWords() const;
    int limitMs() const;
    int score() const;
    int reached() const;
    int rules() const { return stroopy::rules::v1::version; }
    int challengeScore() const { return m_challengeScore; }
    int challengeReached() const { return m_challengeReached; }
    bool isChallenge() const { return m_challenge; }

signals:
    void phaseChanged();
    void countdownChanged();
    void gameChanged();
    void challengeChanged();
    void scoreChanged();
    void answered(bool correct);

private:
    void setPhase(Phase phase);
    void tickCountdown();
    void showChallenge();
    void finish();

    std::optional<stroopy::Game> m_game;
    Mode m_mode = Mode::Classic;
    Phase m_phase = Phase::Idle;
    int m_countdown = 0;
    QTimer m_countdownTimer;
    QTimer m_limitTimer;
    QElapsedTimer m_shown;
    bool m_challenge = false;
    int m_challengeScore = -1;
    int m_challengeReached = -1;
};
