// SPDX-License-Identifier: GPL-3.0-or-later
// Incoming stroopy:// links (docs/spec/game/v1.md §4, docs/spec/multiplayer/v1.md §1.4):
// stroopy://play?c=<game code> and stroopy://join?c=<invitation>. On Android they arrive as
// VIEW intents, which Qt passes to the URL handler registered here.
#pragma once

#include <QObject>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class Links : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Links *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    // Creates the instance and registers it as the handler of the stroopy scheme.
    static Links *instance();
    ~Links() override;

    // Called by QDesktopServices for stroopy:// URLs.
    Q_INVOKABLE void handleUrl(const QUrl &url);

signals:
    // The code as found in the link; Enter code / Join validate it.
    void play(const QString &code);
    void join(const QString &invitation);

private:
    explicit Links(QObject *parent = nullptr);
};
