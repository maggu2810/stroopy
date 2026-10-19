// SPDX-License-Identifier: GPL-3.0-or-later
// Short vibrations (Android); a no-op on platforms without a vibrator.
#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class Haptics : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    using QObject::QObject;

    Q_INVOKABLE static void vibrate(int milliseconds);
};
