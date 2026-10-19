// SPDX-License-Identifier: GPL-3.0-or-later
// QML access to share codes (docs/spec/share-code/v1.md) and the clipboard.
#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

class Sharing : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    using QObject::QObject;

    // seedHex: 16 hex digits. score/reached < 0: no result in the code.
    Q_INVOKABLE static QString encode(int rules, int mode, const QString &seedHex, int score = -1, int reached = -1);
    // Finds a code in arbitrary text. Returns {ok, error ("invalid"|"unsupported"),
    // rules, mode, seedHex, hasResult, score, reached, playable}.
    Q_INVOKABLE static QVariantMap decode(const QString &text);
    Q_INVOKABLE static void copyToClipboard(const QString &text);
    Q_INVOKABLE static QString clipboardText();
};
