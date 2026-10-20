// SPDX-License-Identifier: GPL-3.0-or-later
#include "backup.hpp"

#include "appsettings.hpp"
#include "backup_v1.hpp"
#include "highscores.hpp"

#include <QDateTime>
#include <QQmlEngine>

#include <map>

using namespace Qt::StringLiterals;
namespace bk = stroopy::backup::v1;

Backup *Backup::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    return new Backup{qmlEngine->singletonInstance<Highscores *>("Stroopy", "Highscores"),
                      qmlEngine->singletonInstance<AppSettings *>("Stroopy", "AppSettings")};
}

Backup::Backup(Highscores *highscores, AppSettings *settings) : m_highscores{highscores}, m_settings{settings}
{
}

QString Backup::exportCode() const
{
    bk::Backup backup;
    for (const auto &value : m_highscores->all()) {
        const auto e = value.toMap();
        backup.entries.push_back({.mode = e[u"mode"_s].toInt(), .rules = e[u"rules"_s].toInt(),
                                  .seed = e[u"seedHex"_s].toString().toULongLong(nullptr, 16),
                                  .score = e[u"score"_s].toInt(), .reached = e[u"reached"_s].toInt(),
                                  .time = e[u"time"_s].toDateTime().toSecsSinceEpoch(),
                                  .challenge = e[u"challenge"_s].toBool()});
    }
    // Spec: entries by mode, best first.
    std::ranges::stable_sort(backup.entries, [](const bk::Entry &a, const bk::Entry &b) {
        return a.mode != b.mode ? a.mode < b.mode : a.score != b.score ? a.score > b.score : a.time < b.time;
    });
    backup.settings = bk::Settings{
        .language = m_settings->language().toStdString(),
        .colourBlind = m_settings->colourBlind(),
        .vibration = m_settings->vibration(),
        .name = m_settings->playerName().toStdString(),
        .relays = m_settings->relays().toStdString(),
    };
    return QString::fromStdString(bk::encodeText(backup));
}

QVariantMap Backup::inspect(const QString &text)
{
    const auto decoded = bk::decodeText(text.toStdString());
    if (!decoded) {
        const bool unsupported = decoded.error().kind == bk::Error::Kind::Unsupported;
        return {{u"ok"_s, false}, {u"error"_s, unsupported ? u"unsupported"_s : u"invalid"_s}};
    }
    std::map<int, int> counts;
    for (const auto &e : decoded->entries)
        ++counts[e.mode];
    QVariantList perMode;
    for (const auto &[mode, count] : counts)
        perMode.append(QVariantMap{{u"mode"_s, mode}, {u"count"_s, count}});
    return {
        {u"ok"_s, true},
        {u"entries"_s, static_cast<int>(decoded->entries.size())},
        {u"perMode"_s, perMode},
        {u"hasSettings"_s, decoded->settings.has_value()},
    };
}

int Backup::importCode(const QString &text, bool restoreSettings)
{
    const auto decoded = bk::decodeText(text.toStdString());
    if (!decoded)
        return -1;
    QVariantList entries;
    for (const auto &e : decoded->entries)
        entries.append(QVariantMap{
            {u"rules"_s, e.rules}, {u"mode"_s, e.mode},
            {u"seedHex"_s, u"%1"_s.arg(e.seed, 16, 16, QLatin1Char('0')).toUpper()},
            {u"score"_s, e.score}, {u"reached"_s, e.reached},
            {u"time"_s, QDateTime::fromSecsSinceEpoch(e.time)}, {u"challenge"_s, e.challenge},
        });
    const auto added = m_highscores->merge(entries);

    if (restoreSettings && decoded->settings) {
        const auto &s = *decoded->settings;
        // Unsupported values are ignored by the setters (e.g. an unknown language).
        if (s.language && (s.language->empty() || m_settings->supportedLanguages().contains(
                                                       QString::fromStdString(*s.language))))
            m_settings->setLanguage(QString::fromStdString(*s.language));
        if (s.colourBlind)
            m_settings->setColourBlind(*s.colourBlind);
        if (s.vibration)
            m_settings->setVibration(*s.vibration);
        if (s.name)
            m_settings->setPlayerName(QString::fromStdString(*s.name));
        if (s.relays)
            m_settings->setRelays(QString::fromStdString(*s.relays));
    }
    return added;
}
