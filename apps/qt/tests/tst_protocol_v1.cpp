// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/protocol/v1/*.json
#include "messages.hpp"
#include "nostr.hpp"
#include "protocol_v1.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace stroopy;

namespace {

QJsonObject load(const QString &name)
{
    QFile file{QDir{QStringLiteral(STROOPY_VECTORS_DIR "/protocol/v1")}.filePath(name)};
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open vector file %s", qPrintable(file.fileName()));
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::string str(const QJsonValue &v)
{
    return v.toString().toStdString();
}

// Normalised message as in messages.json.
QJsonValue toJson(const std::optional<net::Message> &m)
{
    if (!m)
        return QJsonValue::Null;
    if (std::holds_alternative<net::Bye>(*m))
        return QJsonObject{{u"type"_s, u"bye"_s}};
    if (const auto *s = std::get_if<net::Start>(&*m))
        return QJsonObject{{u"type"_s, u"start"_s}, {u"rules"_s, s->rules}, {u"mode"_s, s->mode},
                           {u"seed"_s, u"0x%1"_s.arg(s->seed, 16, 16, QLatin1Char('0'))}};
    const auto &s = std::get<net::Status>(*m);
    QJsonArray rules;
    for (const auto r : s.rules)
        rules.append(r);
    QJsonObject o{{u"type"_s, u"status"_s}, {u"name"_s, s.name}, {u"host"_s, s.host}, {u"rules"_s, rules}};
    if (s.mode)
        o[u"mode"_s] = *s.mode;
    if (s.game)
        o[u"game"_s] = QJsonObject{{u"code"_s, s.game->code}, {u"n"_s, s.game->n}, {u"score"_s, s.game->score},
                                   {u"over"_s, s.game->over}};
    return o;
}

} // namespace

class TestProtocolV1 : public QObject {
    Q_OBJECT

private slots:
    void rooms()
    {
        using namespace protocol::v1;
        const auto doc = load(u"rooms.json"_s);
        for (const auto &c : doc[u"encode"_s].toArray()) {
            const auto id = roomIdFromValue(c[u"value"_s].toString().toULongLong(nullptr, 16));
            QCOMPARE(QString::fromStdString(id), c[u"id"_s].toString());
            QCOMPARE(QString::fromStdString(roomCode(id)), c[u"code"_s].toString());
            QCOMPARE(QString::fromStdString(roomTag(id)), c[u"tag"_s].toString());
        }
        for (const auto &c : doc[u"parse"_s].toArray()) {
            const auto r = parseRoom(str(c[u"input"_s]));
            QVERIFY2(r.has_value(), qPrintable(c[u"note"_s].toString()));
            QCOMPARE(QString::fromStdString(*r), c[u"expected"_s].toString());
        }
        for (const auto &c : doc[u"errors"_s].toArray()) {
            const auto r = parseRoom(str(c[u"input"_s]));
            QVERIFY2(!r.has_value(), qPrintable(c[u"note"_s].toString()));
            const bool unsupported = c[u"error"_s].toString() == u"unsupported"_s;
            QVERIFY2((r.error().kind == Error::Kind::Unsupported) == unsupported, qPrintable(c[u"note"_s].toString()));
            if (unsupported)
                QCOMPARE(r.error().version, c[u"version"_s].toInt());
        }
    }

    void events()
    {
        const auto doc = load(u"events.json"_s);
        const auto aux = QByteArray::fromHex(doc[u"auxRand"_s].toString().toLatin1());
        for (const auto &c : doc[u"sign"_s].toArray()) {
            const auto key = net::SecretKey::fromBytes(QByteArray::fromHex(c[u"secretKey"_s].toString().toLatin1()));
            QVERIFY(key);
            const QList<QStringList> tags{
                {u"t"_s, QString::fromStdString(protocol::v1::roomTag(str(c[u"roomId"_s])))}};
            const auto e = key->sign(c[u"createdAt"_s].toInteger(), protocol::v1::kind, tags,
                                     c[u"content"_s].toString(), aux);
            QCOMPARE(QString::fromUtf8(net::serialise(e.pubkey, e.createdAt, e.kind, e.tags, e.content)),
                     c[u"serialised"_s].toString());
            QCOMPARE(e.toJson(), c[u"event"_s].toObject());
            const auto parsed = net::Event::fromJson(c[u"event"_s]);
            QVERIFY(parsed && net::verify(*parsed));
        }
        for (const auto &c : doc[u"reject"_s].toArray()) {
            const auto parsed = net::Event::fromJson(c[u"event"_s]);
            QVERIFY(parsed);
            QVERIFY2(!net::verify(*parsed), qPrintable(c[u"note"_s].toString()));
        }
    }

    void messages()
    {
        for (const auto &c : load(u"messages.json"_s)[u"cases"_s].toArray())
            QVERIFY2(toJson(net::parseMessage(c[u"content"_s].toString())) == c[u"expected"_s],
                     qPrintable(c[u"note"_s].toString()));
    }

    void roundTrip()
    {
        const net::Message m = net::Status{.name = u"Ann"_s, .host = true, .rules = {1}, .mode = 3,
                                           .game = net::GameState{u"SYG1-040G-0000-0000-001A-03CT-T"_s, 4, 3, true}};
        QVERIFY(net::parseMessage(net::encodeMessage(m)) == m);
    }
};

QTEST_GUILESS_MAIN(TestProtocolV1)
#include "tst_protocol_v1.moc"
