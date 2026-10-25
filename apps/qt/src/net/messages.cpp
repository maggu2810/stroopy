// SPDX-License-Identifier: GPL-3.0-or-later
#include "messages.hpp"

#include "game_v1.hpp"
#include "multiplayer_v1.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

using namespace Qt::StringLiterals;

namespace stroopy::net {

namespace {

constexpr std::size_t maxNameLength = 24;

bool isInt(const QJsonValue &v, double lo, double hi = 1e15)
{
    if (!v.isDouble())
        return false;
    const auto d = v.toDouble();
    return d == std::floor(d) && d >= lo && d <= hi;
}

} // namespace

QString cleanName(const QString &name)
{
    std::u32string out;
    for (const auto c : name.toStdU32String())
        if (c >= 0x20 && c != 0x7F)
            out += c;
    const auto trimmed = QString::fromStdU32String(out).trimmed().toStdU32String();
    const auto result = QString::fromStdU32String(trimmed.substr(0, maxNameLength));
    return result.isEmpty() ? u"?"_s : result;
}

std::optional<Message> parseMessage(const QString &content)
{
    const auto doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject())
        return std::nullopt;
    const auto m = doc.object();
    if (!m[u"m"_s].isDouble() || m[u"m"_s].toDouble() != multiplayer::v1::version)
        return std::nullopt;
    const auto type = m[u"type"_s].toString();

    if (type == u"bye"_s)
        return Bye{};

    if (type == u"start"_s) {
        if (!m[u"code"_s].isString())
            return std::nullopt;
        const auto code = m[u"code"_s].toString();
        const auto d = game::v1::decodeText(code.toStdString());
        if (!d || d->result)
            return std::nullopt;
        return Start{.code = code, .rules = d->rules, .mode = d->mode, .seed = d->seed};
    }

    if (type == u"status"_s) {
        if (!m[u"name"_s].isString() || !m[u"host"_s].isBool() || !m[u"rules"_s].isArray())
            return std::nullopt;
        Status s{.name = cleanName(m[u"name"_s].toString()), .host = m[u"host"_s].toBool(), .rules = {},
                 .mode = std::nullopt, .game = std::nullopt};
        for (const auto &r : m[u"rules"_s].toArray()) {
            if (!isInt(r, 1, 255))
                return std::nullopt;
            s.rules.append(r.toInt());
        }
        if (s.rules.isEmpty())
            return std::nullopt;
        if (m.contains(u"mode"_s)) {
            if (!isInt(m[u"mode"_s], 1, 255))
                return std::nullopt;
            s.mode = m[u"mode"_s].toInt();
        }
        if (m.contains(u"game"_s)) {
            const auto g = m[u"game"_s].toObject();
            if (!m[u"game"_s].isObject() || !g[u"code"_s].isString() || !isInt(g[u"n"_s], 0)
                || !isInt(g[u"score"_s], 0) || !g[u"over"_s].isBool())
                return std::nullopt;
            s.game = GameState{.code = g[u"code"_s].toString(), .n = g[u"n"_s].toInt(),
                               .score = g[u"score"_s].toInt(), .over = g[u"over"_s].toBool()};
        }
        return s;
    }
    return std::nullopt;
}

QString encodeMessage(const Message &message)
{
    QJsonObject o{{u"m"_s, multiplayer::v1::version}};
    if (const auto *s = std::get_if<Status>(&message)) {
        o[u"type"_s] = u"status"_s;
        o[u"name"_s] = s->name;
        o[u"host"_s] = s->host;
        QJsonArray rules;
        for (const auto r : s->rules)
            rules.append(r);
        o[u"rules"_s] = rules;
        if (s->mode)
            o[u"mode"_s] = *s->mode;
        if (s->game)
            o[u"game"_s] = QJsonObject{{u"code"_s, s->game->code}, {u"n"_s, s->game->n},
                                       {u"score"_s, s->game->score}, {u"over"_s, s->game->over}};
    } else if (const auto *st = std::get_if<Start>(&message)) {
        o[u"type"_s] = u"start"_s;
        o[u"code"_s] = st->code;
    } else {
        o[u"type"_s] = u"bye"_s;
    }
    return QString::fromUtf8(QJsonDocument{o}.toJson(QJsonDocument::Compact));
}

} // namespace stroopy::net
