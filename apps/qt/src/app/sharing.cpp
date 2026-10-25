// SPDX-License-Identifier: GPL-3.0-or-later
#include "sharing.hpp"

#include "game_v1.hpp"
#include "rules_v1.hpp"

#include <QClipboard>
#include <QGuiApplication>

using namespace Qt::StringLiterals;
namespace sc = stroopy::game::v1;

QString Sharing::encode(int rules, int mode, const QString &seedHex, int score, int reached)
{
    sc::Descriptor d{.rules = rules, .mode = mode, .seed = seedHex.toULongLong(nullptr, 16), .result = std::nullopt};
    if (score >= 0 && reached >= 0)
        d.result = sc::Result{.score = score, .reached = reached};
    return QString::fromStdString(sc::encodeText(d));
}

QVariantMap Sharing::decode(const QString &text)
{
    const auto decoded = sc::decodeText(text.toStdString());
    if (!decoded) {
        const bool unsupported = decoded.error().kind == sc::Error::Kind::Unsupported;
        return {{u"ok"_s, false}, {u"error"_s, unsupported ? u"unsupported"_s : u"invalid"_s}};
    }
    const auto &d = *decoded;
    const bool playable = d.rules == stroopy::rules::v1::version && stroopy::rules::v1::modeFromId(d.mode).has_value();
    return {
        {u"ok"_s, true},
        {u"rules"_s, d.rules},
        {u"mode"_s, d.mode},
        {u"seedHex"_s, u"%1"_s.arg(d.seed, 16, 16, QLatin1Char('0')).toUpper()},
        {u"hasResult"_s, d.result.has_value()},
        {u"score"_s, d.result ? d.result->score : -1},
        {u"reached"_s, d.result ? d.result->reached : -1},
        {u"playable"_s, playable},
    };
}

void Sharing::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

QString Sharing::clipboardText()
{
    return QGuiApplication::clipboard()->text();
}
