// SPDX-License-Identifier: GPL-3.0-or-later
// Live interop driver for docs/spec/multiplayer/v1.md (needs internet; not part of ctest).
// Plays one multiplayer game through the real Multiplayer singleton, without UI:
//   interop_nostr host        creates a room, prints "ROOM <code>", starts once a guest is there
//   interop_nostr join <code> joins the room and plays the game the host starts
// It answers 4 items correctly, then fails at #5, and waits until the other player is out.
// Prints "RESULT <name> <score> <n>" per ranking entry and exits 0 on success.
// Driven by apps/web/tests/e2e/interop-qt.ts.
#include "appsettings.hpp"
#include "game.hpp"
#include "multiplayer.hpp"

#include <QGuiApplication>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <optional>

Q_IMPORT_QML_PLUGIN(StroopyPlugin)

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app{argc, argv};
    QStandardPaths::setTestModeEnabled(true);
    const auto args = app.arguments();
    const bool host = args.value(1) == u"host"_s;
    if (!host && (args.value(1) != u"join"_s || args.size() < 3)) {
        std::fputs("usage: interop_nostr host | join <invitation>\n", stderr);
        return 2;
    }

    QQmlEngine engine;
    auto *settings = engine.singletonInstance<AppSettings *>("Stroopy", "AppSettings");
    settings->setPlayerName(u"QtBot"_s);
    auto *mp = engine.singletonInstance<Multiplayer *>("Stroopy", "Multiplayer");
    auto say = [](const QString &line) {
        std::printf("%s\n", qPrintable(line));
        std::fflush(stdout);
    };

    std::optional<stroopy::Game> game;
    QObject::connect(mp, &Multiplayer::play, &app, [&](int mode, const QString &seedHex) {
        say(u"PLAY %1 %2"_s.arg(mode).arg(seedHex));
        game.emplace(static_cast<stroopy::Game::Mode>(mode), seedHex.toULongLong(nullptr, 16));
        // Play like a person would: one answer every 300 ms, 4 right, then a wrong one.
        auto *timer = new QTimer{&app};
        QObject::connect(timer, &QTimer::timeout, &app, [&, timer] {
            const auto &item = std::get<stroopy::rules::v1::WordItem>(game->current());
            const bool right = game->score() < 4;
            game->answer(right == item.isTrue() ? 1 : 0, 0);
            mp->report(game->n(), game->score(), game->isOver());
            if (game->isOver()) {
                timer->stop();
                mp->setBusy(false);
                say(u"OVER %1 %2"_s.arg(game->score()).arg(game->reached()));
            }
        });
        mp->setBusy(true);
        mp->report(1, 0, false);
        timer->start(300);
    });

    QObject::connect(mp, &Multiplayer::changed, &app, [&] {
        static bool started = false;
        if (host && !started && mp->players().size() >= 2 && mp->canStart()) {
            started = true;
            say(u"GUEST %1"_s.arg(mp->players().value(1).toMap()[u"name"_s].toString()));
            QTimer::singleShot(1000, mp, [mp] {
                mp->setMode(1);
                mp->start();
            });
        }
        const auto ranking = mp->ranking();
        if (game && game->isOver() && ranking.size() >= 2
            && std::ranges::all_of(ranking, [](const QVariant &e) { return e.toMap()[u"over"_s].toBool(); })) {
            for (const auto &e : ranking) {
                const auto m = e.toMap();
                say(u"RESULT %1 %2 %3"_s.arg(m[u"name"_s].toString()).arg(m[u"score"_s].toInt()).arg(m[u"n"_s].toInt()));
            }
            mp->leave();
            QTimer::singleShot(500, &app, [] { QCoreApplication::exit(0); });
        }
    });

    if (host) {
        mp->createRoom();
        say(u"ROOM %1"_s.arg(mp->invitation()));
    } else if (!mp->join(args.value(2))) {
        say(u"bad invitation"_s);
        return 2;
    }
    QTimer::singleShot(120000, &app, [&] {
        say(u"TIMEOUT"_s);
        QCoreApplication::exit(1);
    });
    return app.exec();
}
