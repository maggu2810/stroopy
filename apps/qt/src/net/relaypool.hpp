// SPDX-License-Identifier: GPL-3.0-or-later
// Connections to several Nostr relays (docs/spec/protocol/v1.md §3.3): publish to all,
// subscribe on all, deduplicate and verify incoming events, reconnect with backoff.
#pragma once

#include "nostr.hpp"

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QSet>
#include <QUrl>

class QTimer;
class QWebSocket;

namespace stroopy::net {

class RelayPool : public QObject {
    Q_OBJECT

public:
    RelayPool(const QStringList &urls, QJsonObject filter, QObject *parent = nullptr);
    ~RelayPool() override;

    int connected() const;
    int total() const { return static_cast<int>(m_relays.size()); }
    void publish(const Event &event);

signals:
    void connectedChanged();
    void eventReceived(const stroopy::net::Event &event);

private:
    struct Relay {
        QUrl url;
        QWebSocket *socket = nullptr;
        QTimer *retry = nullptr;
        int backoffMs = 1000;
    };

    void connectRelay(Relay &relay);
    void scheduleRetry(Relay &relay);
    void receive(const QString &frame);

    QList<Relay *> m_relays;
    QJsonObject m_filter;
    QString m_subscription;
    QSet<QString> m_seen;
};

} // namespace stroopy::net
