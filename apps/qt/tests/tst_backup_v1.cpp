// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/backup/v1/*.json
#include "backup_v1.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace stroopy::backup::v1;

namespace {

QJsonObject load(const QString &name)
{
    QFile file{QDir{QStringLiteral(STROOPY_VECTORS_DIR "/backup/v1")}.filePath(name)};
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open vector file %s", qPrintable(file.fileName()));
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::string str(const QJsonValue &v)
{
    return v.toString().toStdString();
}

Backup backup(const QJsonValue &v)
{
    Backup b;
    for (const auto &e : v[u"entries"_s].toArray())
        b.entries.push_back({.mode = e[u"mode"_s].toInt(), .rules = e[u"rules"_s].toInt(),
                             .seed = e[u"seed"_s].toString().toULongLong(nullptr, 16),
                             .score = e[u"score"_s].toInt(), .reached = e[u"reached"_s].toInt(),
                             .time = e[u"time"_s].toInteger(), .challenge = e[u"challenge"_s].toBool()});
    if (const auto s = v[u"settings"_s]; s.isObject()) {
        Settings settings;
        if (s.toObject().contains(u"1"_s))
            settings.language = str(s[u"1"_s]);
        if (s.toObject().contains(u"2"_s))
            settings.colourBlind = s[u"2"_s].toBool();
        if (s.toObject().contains(u"3"_s))
            settings.vibration = s[u"3"_s].toBool();
        if (s.toObject().contains(u"4"_s))
            settings.name = str(s[u"4"_s]);
        if (s.toObject().contains(u"5"_s))
            settings.relays = str(s[u"5"_s]);
        b.settings = settings;
    }
    return b;
}

} // namespace

class TestBackupV1 : public QObject {
    Q_OBJECT

private slots:
    void encode()
    {
        for (const auto &c : load(u"encode.json"_s)[u"cases"_s].toArray()) {
            const auto note = c[u"note"_s].toString();
            const auto b = backup(c);
            const auto record = encodeRecord(b);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(record.data()), qsizetype(record.size())).toHex(),
                     c[u"record"_s].toString().toLatin1());
            QCOMPARE(QString::fromStdString(encodeText(b)), c[u"text"_s].toString());

            auto expected = b;
            for (auto &e : expected.entries) {
                e.score = std::min(e.score, 0xFFFF);
                e.reached = std::min(e.reached, 0xFFFF);
            }
            const auto decoded = decodeText(str(c[u"text"_s]));
            QVERIFY2(decoded.has_value(), qPrintable(note));
            QVERIFY2(*decoded == expected, qPrintable(note));
        }
    }

    void decodeValid()
    {
        for (const auto &c : load(u"decode.json"_s)[u"valid"_s].toArray()) {
            const auto note = c[u"note"_s].toString();
            const auto decoded = decodeText(str(c[u"input"_s]));
            QVERIFY2(decoded.has_value(), qPrintable(note));
            QVERIFY2(*decoded == backup(c[u"expected"_s]), qPrintable(note));
        }
    }

    void decodeErrors()
    {
        for (const auto &c : load(u"decode.json"_s)[u"errors"_s].toArray()) {
            const auto note = c[u"note"_s].toString();
            const auto decoded = decodeText(str(c[u"input"_s]));
            QVERIFY2(!decoded.has_value(), qPrintable(note));
            const auto kind = c[u"error"_s].toString() == u"unsupported"_s ? Error::Kind::Unsupported
                                                                            : Error::Kind::Invalid;
            QVERIFY2(decoded.error().kind == kind, qPrintable(note));
            if (kind == Error::Kind::Unsupported)
                QCOMPARE(decoded.error().version, c[u"version"_s].toInt());
        }
    }
};

QTEST_GUILESS_MAIN(TestBackupV1)
#include "tst_backup_v1.moc"
