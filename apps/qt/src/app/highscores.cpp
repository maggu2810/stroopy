// SPDX-License-Identifier: GPL-3.0-or-later
#include "highscores.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {

QString filePath()
{
    return QDir{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)}.filePath(u"highscores.json"_s);
}

// Better first: higher score, then earlier date.
bool better(const QVariant &a, const QVariant &b)
{
    const auto ma = a.toMap();
    const auto mb = b.toMap();
    if (ma[u"score"_s].toInt() != mb[u"score"_s].toInt())
        return ma[u"score"_s].toInt() > mb[u"score"_s].toInt();
    return ma[u"time"_s].toDateTime() < mb[u"time"_s].toDateTime();
}

} // namespace

Highscores::Highscores(QObject *parent) : QObject{parent}
{
    load();
}

QVariantList Highscores::entries(int mode) const
{
    QVariantList out;
    std::ranges::copy_if(m_entries, std::back_inserter(out),
                         [mode](const QVariant &e) { return e.toMap()[u"mode"_s].toInt() == mode; });
    std::ranges::stable_sort(out, better);
    return out;
}

int Highscores::best(int mode) const
{
    const auto list = entries(mode);
    return list.isEmpty() ? 0 : list.first().toMap()[u"score"_s].toInt();
}

int Highscores::submit(int rules, int mode, const QString &seedHex, int score, int reached, bool challenge)
{
    if (score <= 0)
        return 0;
    const QVariantMap entry{
        {u"rules"_s, rules}, {u"mode"_s, mode}, {u"seedHex"_s, seedHex}, {u"score"_s, score},
        {u"reached"_s, reached}, {u"time"_s, QDateTime::currentDateTime()}, {u"challenge"_s, challenge},
    };
    auto list = entries(mode);
    const auto pos = std::ranges::upper_bound(list, QVariant{entry}, better) - list.begin();
    if (pos >= maxEntriesPerMode)
        return 0;
    list.insert(pos, entry);
    list.resize(std::min<qsizetype>(list.size(), maxEntriesPerMode));

    m_entries.removeIf([mode](const QVariant &e) { return e.toMap()[u"mode"_s].toInt() == mode; });
    m_entries.append(list);
    save();
    ++m_revision;
    emit changed();
    return static_cast<int>(pos) + 1;
}

void Highscores::clear()
{
    m_entries.clear();
    save();
    ++m_revision;
    emit changed();
}

void Highscores::load()
{
    QFile file{filePath()};
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    for (const auto &value : root[u"entries"_s].toArray()) {
        auto map = value.toObject().toVariantMap();
        map[u"time"_s] = QDateTime::fromString(map[u"time"_s].toString(), Qt::ISODate);
        m_entries.append(map);
    }
}

void Highscores::save() const
{
    QJsonArray array;
    for (const auto &e : m_entries) {
        auto map = e.toMap();
        map[u"time"_s] = map[u"time"_s].toDateTime().toString(Qt::ISODate);
        array.append(QJsonObject::fromVariantMap(map));
    }
    QDir{}.mkpath(QFileInfo{filePath()}.absolutePath());
    QSaveFile file{filePath()};
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument{QJsonObject{{u"version"_s, 1}, {u"entries"_s, array}}}.toJson());
        file.commit();
    }
}
