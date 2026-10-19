// SPDX-License-Identifier: GPL-3.0-or-later
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app{argc, argv};
    QGuiApplication::setApplicationName(u"Stroopy"_s);
    QGuiApplication::setOrganizationDomain(u"maggu2810.de"_s);
    QQuickStyle::setStyle(u"Material"_s);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(u"Stroopy"_s, u"Main"_s);
    return QGuiApplication::exec();
}
