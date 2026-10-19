// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamepad.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <QQmlEngine>

#include <optional>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#endif

namespace {

// android.view.KeyEvent key codes
std::optional<Gamepad::Button> fromAndroid(int keyCode)
{
    switch (keyCode) {
    case 96: return Gamepad::Button::A;
    case 97: return Gamepad::Button::B;
    case 99: return Gamepad::Button::X;
    case 100: return Gamepad::Button::Y;
    case 102: return Gamepad::Button::L1;
    case 103: return Gamepad::Button::R1;
    case 108: return Gamepad::Button::Start;
    case 109: return Gamepad::Button::Select;
    default: return std::nullopt;
    }
}

Gamepad *s_instance = nullptr;

} // namespace

#ifdef Q_OS_ANDROID
// JNI callback of StroopyActivity.onGamepadButton (must not live in an anonymous namespace).
static void onGamepadButton(JNIEnv *, jclass, jint keyCode, jboolean pressed)
{
    Gamepad::handleAndroidKey(keyCode, pressed);
}
Q_DECLARE_JNI_NATIVE_METHOD(onGamepadButton)
#endif

Gamepad::Gamepad(QObject *parent) : QObject{parent}
{
    s_instance = this;
}

Gamepad *Gamepad::create(QQmlEngine *, QJSEngine *)
{
    auto *gamepad = instance();
    QQmlEngine::setObjectOwnership(gamepad, QQmlEngine::CppOwnership);
    return gamepad;
}

Gamepad *Gamepad::instance()
{
    if (!s_instance)
        new Gamepad{QCoreApplication::instance()};
    return s_instance;
}

void Gamepad::handleAndroidKey(int keyCode, bool pressed)
{
    // Called on the Android UI thread; hand over to the Qt thread.
    QMetaObject::invokeMethod(instance(), [keyCode, pressed] { instance()->handle(keyCode, pressed); },
                              Qt::QueuedConnection);
}

void Gamepad::handle(int keyCode, bool down)
{
    const auto button = fromAndroid(keyCode);
    if (!button)
        return;
    if (*button == Button::L1 || *button == Button::R1) {
        const bool before = shoulderHeld();
        (*button == Button::L1 ? m_l1 : m_r1) = down;
        if (before != shoulderHeld())
            emit shoulderHeldChanged();
    }
    if (down)
        emit pressed(*button);
}

void Gamepad::registerNatives()
{
#ifdef Q_OS_ANDROID
    QJniEnvironment env;
    env.registerNativeMethods("de/maggu2810/android/stroopy/qt/StroopyActivity",
                              {Q_JNI_NATIVE_METHOD(onGamepadButton)});
#endif
}
