// SPDX-License-Identifier: GPL-3.0-or-later
#include "links.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

namespace {

Links *s_instance = nullptr;

// Per user: in the runtime directory (XDG_RUNTIME_DIR on Linux).
QString socketName()
{
    return QDir{QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)}.filePath(u"stroopy-links"_s);
}

// A link plus some slack; anything longer is not one of ours.
constexpr qint64 maxLinkSize = 4096;

} // namespace

Links::Links(QObject *parent) : QObject{parent}
{
    s_instance = this;
    QDesktopServices::setUrlHandler(u"stroopy"_s, this, "handleUrl");
}

Links::~Links()
{
    QDesktopServices::unsetUrlHandler(u"stroopy"_s);
    s_instance = nullptr;
}

Links *Links::create(QQmlEngine *, QJSEngine *)
{
    auto *links = instance();
    QQmlEngine::setObjectOwnership(links, QQmlEngine::CppOwnership);
    return links;
}

Links *Links::instance()
{
    if (!s_instance)
        new Links{QCoreApplication::instance()};
    return s_instance;
}

QUrl Links::fromArguments(const QStringList &arguments)
{
    for (const auto &argument : arguments.sliced(qMin(1, arguments.size()))) {
        const QUrl url{argument};
        if (url.scheme() == u"stroopy"_s)
            return url;
    }
    return {};
}

bool Links::forward(const QUrl &url)
{
    QLocalSocket socket;
    socket.connectToServer(socketName());
    if (!socket.waitForConnected(1000))
        return false;
    socket.write(url.toEncoded() + '\n');
    return socket.waitForBytesWritten(1000);
}

void Links::listen()
{
    if (m_server)
        return;
    m_server = new QLocalServer{this};
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, [this] {
        while (auto *socket = m_server->nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
                if (socket->canReadLine()) {
                    handleUrl(QUrl::fromEncoded(socket->readLine(maxLinkSize).trimmed()));
                    socket->disconnectFromServer();
                } else if (socket->bytesAvailable() > maxLinkSize) {
                    socket->abort();
                }
            });
        }
    });
    if (m_server->listen(socketName()))
        return;
    // Left over from a crashed instance? Another running instance keeps the socket.
    QLocalSocket probe;
    probe.connectToServer(socketName());
    if (probe.waitForConnected(500))
        return;
    QLocalServer::removeServer(socketName());
    m_server->listen(socketName());
}

void Links::handleUrl(const QUrl &url)
{
    // Only the c parameter counts (game §4, multiplayer §1.4); the pages decode it.
    if (url.scheme() != u"stroopy"_s)
        return;
    const auto code = QUrlQuery{url}.queryItemValue(u"c"_s, QUrl::FullyDecoded);
    if (code.isEmpty())
        return;
    if (url.host() == u"play"_s)
        emit play(code);
    else if (url.host() == u"join"_s)
        emit join(code);
}
