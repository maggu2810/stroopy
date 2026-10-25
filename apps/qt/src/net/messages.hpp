// SPDX-License-Identifier: GPL-3.0-or-later
// Multiplayer v1 messages (docs/spec/multiplayer/v1.md §2).
#pragma once

#include <QList>
#include <QString>

#include <optional>
#include <variant>

namespace stroopy::net {

struct GameState {
    QString code; // game code of the game, without result
    int n = 0;
    int score = 0;
    bool over = false;
    bool operator==(const GameState &) const = default;
};

struct Status {
    QString name;
    bool host = false;
    QList<int> rules;
    std::optional<int> mode;
    std::optional<GameState> game;
    bool operator==(const Status &) const = default;
};

struct Start {
    QString code;
    int rules = 0;
    int mode = 0;
    quint64 seed = 0;
    bool operator==(const Start &) const = default;
};

struct Bye {
    bool operator==(const Bye &) const = default;
};

using Message = std::variant<Status, Start, Bye>;

// Removes control characters, trims, keeps at most 24 code points; "" becomes "?".
QString cleanName(const QString &name);
// Event content -> message, or std::nullopt if it must be ignored.
std::optional<Message> parseMessage(const QString &content);
QString encodeMessage(const Message &message);

} // namespace stroopy::net
