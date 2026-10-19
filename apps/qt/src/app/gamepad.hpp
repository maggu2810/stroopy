// SPDX-License-Identifier: GPL-3.0-or-later
// Gamepad buttons that Qt does not deliver as key events (docs/design/product.md, "Input").
// On Android they are forwarded by StroopyActivity via JNI. The D-pad arrives as arrow keys.
#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class Gamepad : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // L1 or R1 held: selects the outer ring of colours in Find mode.
    Q_PROPERTY(bool shoulderHeld READ shoulderHeld NOTIFY shoulderHeldChanged)

public:
    enum class Button { A, B, X, Y, L1, R1, Start, Select };
    Q_ENUM(Button)

    static Gamepad *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static Gamepad *instance();
    // Android KeyEvent key code; safe to call from any thread.
    static void handleAndroidKey(int keyCode, bool pressed);
    // Registers the JNI native method of StroopyActivity (Android only, no-op elsewhere).
    static void registerNatives();

    bool shoulderHeld() const { return m_l1 || m_r1; }

signals:
    void pressed(Gamepad::Button button);
    void shoulderHeldChanged();

private:
    explicit Gamepad(QObject *parent = nullptr);
    void handle(int keyCode, bool down);

    bool m_l1 = false;
    bool m_r1 = false;
};
