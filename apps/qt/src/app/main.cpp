// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamepad.hpp"
#include "links.hpp"

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
    QGuiApplication::setDesktopFileName(u"de.maggu2810.android.stroopy.qt"_s);
    QQuickStyle::setStyle(u"Material"_s);
    Gamepad::registerNatives();
    // Before the event loop: Android hands over the launching intent once it runs.
    [[maybe_unused]] auto *links = Links::instance();
#ifndef Q_OS_ANDROID
    // Desktop: a link as argument goes to the running instance, if any, else is opened here.
    if (const auto url = Links::fromArguments(QCoreApplication::arguments()); !url.isEmpty()) {
        if (Links::forward(url))
            return EXIT_SUCCESS;
        QMetaObject::invokeMethod(links, [links, url] { links->handleUrl(url); }, Qt::QueuedConnection);
    }
    links->listen();
#endif

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(u"Stroopy"_s, u"Main"_s);
    return QGuiApplication::exec();
}
