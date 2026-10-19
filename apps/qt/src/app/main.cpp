// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamepad.hpp"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(StroopyPlugin)

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app{argc, argv};
    QGuiApplication::setApplicationName(u"Stroopy"_s);
    QGuiApplication::setOrganizationDomain(u"maggu2810.de"_s);
    QGuiApplication::setApplicationVersion(QStringLiteral(STROOPY_VERSION));
    QGuiApplication::setWindowIcon(QIcon{u":/icons/stroopy.png"_s});
    QQuickStyle::setStyle(u"Material"_s);
    Gamepad::registerNatives();

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(u"Stroopy"_s, u"Main"_s);
    return QGuiApplication::exec();
}
