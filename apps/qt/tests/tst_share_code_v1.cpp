// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/share-code/v1/*.json
#include "share_code_v1.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace stroopy::share_code::v1;

namespace {

QJsonObject load(const QString &name)
{
    QFile file{QDir{QStringLiteral(STROOPY_VECTORS_DIR "/share-code/v1")}.filePath(name)};
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open vector file %s", qPrintable(file.fileName()));
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::uint64_t u64(const QJsonValue &v)
{
    return v.toString().toULongLong(nullptr, 16);
}

Descriptor descriptor(const QJsonValue &v)
{
    Descriptor d{.rules = v[u"rules"_s].toInt(), .mode = v[u"mode"_s].toInt(), .seed = u64(v[u"seed"_s]),
                 .result = std::nullopt};
    if (v.toObject().contains(u"result"_s))
        d.result = Result{.score = v[u"result"_s][u"score"_s].toInt(), .reached = v[u"result"_s][u"reached"_s].toInt()};
    return d;
}

std::string str(const QJsonValue &v)
{
    return v.toString().toStdString();
}

} // namespace

class TestShareCodeV1 : public QObject {
    Q_OBJECT

private slots:
    void crc()
    {
        const auto doc = load(u"encode.json"_s);
        const auto check = doc[u"crc16Check"_s];
        QCOMPARE(crc16(str(check[u"input"_s])), static_cast<std::uint16_t>(u64(check[u"crc"_s])));
    }

    void encode()
    {
        for (const auto &c : load(u"encode.json"_s)[u"cases"_s].toArray()) {
            const auto d = descriptor(c);
            const auto record = encodeRecord(d);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(record.data()), qsizetype(record.size())).toHex(),
                     c[u"record"_s].toString().toLatin1());
            QCOMPARE(QString::fromStdString(encodeText(d)), c[u"text"_s].toString());

            auto expected = d;
            if (expected.result)
                expected.result = Result{std::min(d.result->score, 0xFFFF), std::min(d.result->reached, 0xFFFF)};
            const auto decoded = decodeText(str(c[u"text"_s]));
            QVERIFY(decoded.has_value());
            QVERIFY(*decoded == expected);
        }
    }

    void decodeValid()
    {
        for (const auto &c : load(u"decode.json"_s)[u"valid"_s].toArray()) {
            const auto decoded = decodeText(str(c[u"input"_s]));
            QVERIFY2(decoded.has_value(), qPrintable(c[u"note"_s].toString()));
            QVERIFY2(*decoded == descriptor(c[u"expected"_s]), qPrintable(c[u"note"_s].toString()));
        }
    }

    void decodeErrors()
    {
        for (const auto &c : load(u"decode.json"_s)[u"errors"_s].toArray()) {
            const auto decoded = decodeText(str(c[u"input"_s]));
            QVERIFY2(!decoded.has_value(), qPrintable(c[u"note"_s].toString()));
            const auto kind = c[u"error"_s].toString() == u"unsupported"_s ? Error::Kind::Unsupported
                                                                            : Error::Kind::Invalid;
            QVERIFY2(decoded.error().kind == kind, qPrintable(c[u"note"_s].toString()));
            if (kind == Error::Kind::Unsupported)
                QCOMPARE(decoded.error().version, c[u"version"_s].toInt());
        }
    }
};

QTEST_GUILESS_MAIN(TestShareCodeV1)
#include "tst_share_code_v1.moc"
