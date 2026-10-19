// SPDX-License-Identifier: GPL-3.0-or-later
#include "haptics.hpp"

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#endif

void Haptics::vibrate(int milliseconds)
{
#ifdef Q_OS_ANDROID
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const auto vibrator = context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
                                                   QJniObject::fromString(QStringLiteral("vibrator")).object());
    if (!vibrator.isValid())
        return;
    const auto effect = QJniObject::callStaticObjectMethod("android/os/VibrationEffect", "createOneShot",
                                                           "(JI)Landroid/os/VibrationEffect;",
                                                           static_cast<jlong>(milliseconds), jint{-1});
    vibrator.callMethod<void>("vibrate", "(Landroid/os/VibrationEffect;)V", effect.object());
#else
    Q_UNUSED(milliseconds)
#endif
}
