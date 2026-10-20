// SPDX-License-Identifier: GPL-3.0-or-later
#include "relaypool.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTimer>
#include <QWebSocket>

using namespace Qt::StringLiterals;

namespace stroopy::net {

namespace {
constexpr int maxBackoffMs = 30000;
constexpr qsizetype seenLimit = 2000;
} // namespace

RelayPool::RelayPool(const QStringList &urls, QJsonObject filter, QObject *parent)
    : QObject{parent}, m_filter{std::move(filter)},
      m_subscription{u"stroopy-%1"_s.arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'))}
{
    for (const auto &url : urls) {
        auto *relay = new Relay{.url = QUrl{url}};
        relay->retry = new QTimer{this};
        relay->retry->setSingleShot(true);
        connect(relay->retry, &QTimer::timeout, this, [this, relay] { connectRelay(*relay); });
        m_relays.append(relay);
        connectRelay(*relay);
    }
}

RelayPool::~RelayPool()
{
    for (auto *relay : std::as_const(m_relays)) {
        if (relay->socket) {
            relay->socket->disconnect(this);
            relay->socket->close();
        }
        delete relay;
    }
}

int RelayPool::connected() const
{
    return static_cast<int>(std::ranges::count_if(m_relays, [](const Relay *r) {
        return r->socket && r->socket->state() == QAbstractSocket::ConnectedState;
    }));
}

void RelayPool::publish(const Event &event)
{
    m_seen.insert(event.id);
    const auto frame = QString::fromUtf8(
        QJsonDocument{QJsonArray{u"EVENT"_s, event.toJson()}}.toJson(QJsonDocument::Compact));
    for (auto *relay : std::as_const(m_relays))
        if (relay->socket && relay->socket->state() == QAbstractSocket::ConnectedState)
            relay->socket->sendTextMessage(frame);
}

void RelayPool::connectRelay(Relay &relay)
{
    if (!relay.url.isValid() || (relay.url.scheme() != u"wss"_s && relay.url.scheme() != u"ws"_s))
        return; // not a relay URL; nothing to retry
    auto *socket = new QWebSocket{QString{}, QWebSocketProtocol::VersionLatest, this};
    relay.socket = socket;
    connect(socket, &QWebSocket::connected, this, [this, &relay, socket] {
        relay.backoffMs = 1000;
        socket->sendTextMessage(QString::fromUtf8(
            QJsonDocument{QJsonArray{u"REQ"_s, m_subscription, m_filter}}.toJson(QJsonDocument::Compact)));
        emit connectedChanged();
    });
    connect(socket, &QWebSocket::textMessageReceived, this, &RelayPool::receive);
    connect(socket, &QWebSocket::disconnected, this, [this, &relay, socket] {
        if (relay.socket == socket)
            relay.socket = nullptr;
        socket->deleteLater();
        emit connectedChanged();
        scheduleRetry(relay);
    });
    socket->open(relay.url);
}

void RelayPool::scheduleRetry(Relay &relay)
{
    relay.retry->start(relay.backoffMs);
    relay.backoffMs = std::min(relay.backoffMs * 2, maxBackoffMs);
}

void RelayPool::receive(const QString &text)
{
    // OK, EOSE, NOTICE and CLOSED are informative only.
    const auto frame = QJsonDocument::fromJson(text.toUtf8()).array();
    if (frame.size() < 3 || frame[0] != u"EVENT"_s || frame[1] != m_subscription)
        return;
    const auto event = Event::fromJson(frame[2]);
    if (!event || m_seen.contains(event->id) || !verify(*event))
        return;
    if (m_seen.size() > seenLimit)
        m_seen.clear(); // old duplicates are harmless: status messages are idempotent
    m_seen.insert(event->id);
    emit eventReceived(*event);
}

} // namespace stroopy::net
