// SPDX-License-Identifier: GPL-3.0-or-later
// Live multiplayer over Nostr (docs/spec/multiplayer/v1.md §4, docs/design/product.md "Multiplayer"):
// the current room, its players, the host, starting games and everybody's progress.
// The game itself runs locally (GameController); this only reports.
#pragma once

#include "messages.hpp"
#include "multiplayer_v1.hpp"
#include "nostr.hpp"

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include <functional>
#include <memory>
#include <optional>

class AppSettings;
class QQmlEngine;
class QJSEngine;

namespace stroopy::net {
class RelayPool;
}

class Multiplayer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool active READ active NOTIFY changed)
    // The room's invitation (SYM text code); every player may share it.
    Q_PROPERTY(QString invitation READ invitation NOTIFY changed)
    // The room's relays, from the invitation.
    Q_PROPERTY(QStringList relays READ relays NOTIFY changed)
    Q_PROPERTY(int connected READ connected NOTIFY changed)
    Q_PROPERTY(int relayCount READ relayCount NOTIFY changed)
    Q_PROPERTY(bool isHost READ isHost NOTIFY changed)
    Q_PROPERTY(QString hostName READ hostName NOTIFY changed)
    // Host's selected mode for the next game.
    Q_PROPERTY(int mode READ mode NOTIFY changed)
    // Everyone, me first: {name, me, host}.
    Q_PROPERTY(QVariantList players READ players NOTIFY changed)
    // Everyone in my last/current game, best first: {name, me, score, n, over}.
    Q_PROPERTY(QVariantList ranking READ ranking NOTIFY changed)
    // Mode of my last/current game, 0 if none.
    Q_PROPERTY(int lastMode READ lastMode NOTIFY changed)
    // Others in my current game: {name, score, n, over}.
    Q_PROPERTY(QVariantList opponents READ opponents NOTIFY changed)
    // The host started a newer game while I was playing.
    Q_PROPERTY(bool pending READ pending NOTIFY changed)
    Q_PROPERTY(bool canStart READ canStart NOTIFY changed)

public:
    static Multiplayer *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    ~Multiplayer() override;

    // Invitation in arbitrary text: {ok, error ("invalid"|"unsupported"|"noTransport")}.
    Q_INVOKABLE static QVariantMap parseInvitation(const QString &text);
    Q_INVOKABLE void createRoom();
    Q_INVOKABLE bool join(const QString &text);
    Q_INVOKABLE void leave();
    Q_INVOKABLE void setMode(int mode);
    // Host: start a new game for everybody.
    Q_INVOKABLE void start();
    Q_INVOKABLE void joinPending();
    // The local game's progress; call on every change. `busy`: countdown or game running.
    Q_INVOKABLE void report(int n, int score, bool over);
    Q_INVOKABLE void setBusy(bool busy);
    // Message to invite others: "Play Stroopy with me: stroopy://join?c=SYM1-…"
    Q_INVOKABLE QString inviteText() const;

    bool active() const { return m_pool != nullptr; }
    QString invitation() const;
    QStringList relays() const;
    int connected() const;
    int relayCount() const;
    bool isHost() const;
    QString hostName() const;
    int mode() const { return m_mode; }
    QVariantList players() const;
    QVariantList ranking() const;
    int lastMode() const;
    QVariantList opponents() const;
    bool pending() const { return m_pending.has_value(); }
    bool canStart() const;

signals:
    void changed();
    // A room was created or joined: show the lobby.
    void opened();
    // Start the local game (GameController.start).
    void play(int mode, const QString &seedHex);

private:
    struct Player {
        QString key;
        stroopy::net::Status status;
        qint64 lastSeen = 0;
    };

    explicit Multiplayer(AppSettings *settings);
    void open(const stroopy::multiplayer::v1::Invitation &invitation, bool asHost);
    void close();
    QList<const Player *> everyone() const;
    QString hostKey() const;
    void begin(const stroopy::net::Start &game);
    void offer(const stroopy::net::Start &game);
    void sendStatus(bool urgent = false);
    void send(const stroopy::net::Message &message);
    void tick();
    void removePlayers(const std::function<bool(const Player &)> &gone);
    void receive(const stroopy::net::Event &event);

    AppSettings *m_settings;
    stroopy::net::RelayPool *m_pool = nullptr;
    std::optional<stroopy::net::SecretKey> m_secret;
    stroopy::multiplayer::v1::Invitation m_invitation;
    Player m_me;
    QList<Player> m_players; // others
    QList<Player> m_departed; // left after playing; their results stay in the ranking
    QSet<QString> m_played;
    std::optional<stroopy::net::Start> m_pending;
    int m_mode = 1;
    bool m_busy = false;
    qint64 m_lastSent = 0;
    qint64 m_lastHostSeen = 0;
    QTimer m_heartbeat;
    QTimer m_statusTimer;
};
