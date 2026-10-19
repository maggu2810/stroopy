// SPDX-License-Identifier: GPL-3.0-or-later
// Conformance: docs/spec/test-vectors/rules/v1/*.json
#include "game.hpp"
#include "rules_v1.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;

using namespace stroopy::rules::v1;

namespace {

QJsonObject load(const QString &name)
{
    QFile file{QDir{QStringLiteral(STROOPY_VECTORS_DIR "/rules/v1")}.filePath(name)};
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open vector file %s", qPrintable(file.fileName()));
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::uint64_t u64(const QJsonValue &v)
{
    bool ok = false;
    const auto value = v.toString().toULongLong(&ok, 16);
    if (!ok)
        qFatal("bad u64 hex value %s", qPrintable(v.toString()));
    return value;
}

std::uint64_t u64Int(const QJsonValue &v)
{
    return static_cast<std::uint64_t>(v.toDouble()); // up to 2^32, exact in double
}

} // namespace

class TestRulesV1 : public QObject {
    Q_OBJECT

private slots:
    void splitmix64()
    {
        for (const auto &c : load(u"splitmix64.json"_s)[u"cases"_s].toArray()) {
            SplitMix64 rng{u64(c[u"seed"_s])};
            for (const auto &expected : c[u"next"_s].toArray())
                QCOMPARE(rng.next(), u64(expected));
        }
    }

    void bounded()
    {
        for (const auto &c : load(u"bounded.json"_s)[u"cases"_s].toArray()) {
            SplitMix64 rng{u64(c[u"seed"_s])};
            const auto k = u64Int(c[u"k"_s]);
            const bool hasExcept = c.toObject().contains(u"except"_s);
            for (const auto &expected : c[u"values"_s].toArray()) {
                const auto actual = hasExcept ? rng.belowExcept(k, u64Int(c[u"except"_s])) : rng.below(k);
                QCOMPARE(actual, u64Int(expected));
            }
        }
    }

    void timing()
    {
        const auto doc = load(u"timing.json"_s);
        QCOMPARE(chronoTotalMs, doc[u"chronoTotalMs"_s].toInt());
        for (const auto &e : doc[u"itemLimit"_s].toArray())
            QCOMPARE(itemLimitMs(e[u"n"_s].toInt()), e[u"ms"_s].toInt());
        for (const auto &e : doc[u"roundLimit"_s].toArray())
            QCOMPARE(roundLimitMs(e[u"n"_s].toInt()), e[u"ms"_s].toInt());
    }

    void wordItems_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("classic") << u"items-classic.json"_s;
        QTest::newRow("chrono") << u"items-chrono.json"_s;
        QTest::newRow("find") << u"items-find.json"_s;
    }

    void wordItems()
    {
        QFETCH(QString, file);
        const auto doc = load(file);
        const auto mode = modeFromId(doc[u"modeId"_s].toInt());
        QVERIFY(mode.has_value());
        for (const auto &c : doc[u"cases"_s].toArray()) {
            WordItemGenerator gen{*mode, u64(c[u"seed"_s])};
            for (const auto &e : c[u"items"_s].toArray()) {
                const auto item = gen.next();
                QCOMPARE(item.n, e[u"n"_s].toInt());
                QCOMPARE(item.pool, e[u"pool"_s].toInt());
                QCOMPARE(item.ink, e[u"ink"_s].toInt());
                QCOMPARE(item.word, e[u"word"_s].toInt());
            }
        }
    }

    void tapRounds()
    {
        for (const auto &c : load(u"rounds-tap.json"_s)[u"cases"_s].toArray()) {
            TapRoundGenerator gen{u64(c[u"seed"_s])};
            for (const auto &e : c[u"rounds"_s].toArray()) {
                const auto round = gen.next();
                QCOMPARE(round.n, e[u"n"_s].toInt());
                QCOMPARE(round.pool, e[u"pool"_s].toInt());
                QCOMPARE(round.trueCard, e[u"trueCard"_s].toInt());
                for (int i = 0; i < 4; ++i) {
                    QCOMPARE(round.inks[i], e[u"inks"_s][i].toInt());
                    QCOMPARE(round.words[i], e[u"words"_s][i].toInt());
                }
            }
        }
    }

    // Game session (spec §6) on top of the generators.
    void classicScoringAndEnd()
    {
        stroopy::Game game{Mode::Classic, 42};
        for (int i = 0; i < 5; ++i) {
            const auto &item = std::get<WordItem>(game.current());
            QCOMPARE(game.answer(item.isTrue() ? 1 : 0, 100), stroopy::Game::Outcome::Correct);
        }
        const auto &item = std::get<WordItem>(game.current());
        QCOMPARE(game.answer(item.isTrue() ? 0 : 1, 100), stroopy::Game::Outcome::GameOver);
        QVERIFY(game.isOver());
        QCOMPARE(game.score(), 5);
        QCOMPARE(game.reached(), 6);
    }

    void classicTimeoutAtLimit()
    {
        stroopy::Game game{Mode::Classic, 42};
        const auto &item = std::get<WordItem>(game.current());
        QCOMPARE(game.answer(item.isTrue() ? 1 : 0, itemLimitMs(1)), stroopy::Game::Outcome::GameOver);
        QCOMPARE(game.score(), 0);
        QCOMPARE(game.reached(), 1);
    }

    void chronoNeverBelowZero()
    {
        stroopy::Game game{Mode::Chrono, 7};
        const auto &first = std::get<WordItem>(game.current());
        QCOMPARE(game.answer(first.isTrue() ? 0 : 1, 0), stroopy::Game::Outcome::Wrong);
        QCOMPARE(game.score(), 0);
        const auto &second = std::get<WordItem>(game.current());
        QCOMPARE(game.answer(second.isTrue() ? 1 : 0, 0), stroopy::Game::Outcome::Correct);
        QCOMPARE(game.score(), 1);
        QCOMPARE(game.n(), 3);
        game.timeout();
        QVERIFY(game.isOver());
        QCOMPARE(game.reached(), 3);
    }

    void tapAndFindCorrectChoice()
    {
        stroopy::Game tap{Mode::Tap, 1};
        QCOMPARE(tap.answer(std::get<TapRound>(tap.current()).trueCard, 10), stroopy::Game::Outcome::Correct);
        stroopy::Game find{Mode::Find, 1};
        const auto &item = std::get<WordItem>(find.current());
        QVERIFY(!item.isTrue());
        QCOMPARE(find.answer(item.word, 10), stroopy::Game::Outcome::Correct);
    }
};

QTEST_GUILESS_MAIN(TestRulesV1)
#include "tst_rules_v1.moc"
