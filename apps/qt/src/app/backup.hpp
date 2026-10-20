// SPDX-License-Identifier: GPL-3.0-or-later
// Export and import of the backup code (docs/spec/backup/v1.md, docs/design/product.md "Backup").
#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

class AppSettings;
class Highscores;
class QQmlEngine;
class QJSEngine;

class Backup : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Backup *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // All highscores and shared settings as a backup code.
    Q_INVOKABLE QString exportCode() const;
    // Finds a backup in arbitrary text. Returns {ok, error ("invalid"|"unsupported"),
    // entries (total), perMode (list of {mode, count}), hasSettings}.
    Q_INVOKABLE static QVariantMap inspect(const QString &text);
    // Merges the highscores and optionally applies the settings. Returns the number of
    // highscores added, or -1 if the text holds no valid backup.
    Q_INVOKABLE int importCode(const QString &text, bool restoreSettings);

private:
    Backup(Highscores *highscores, AppSettings *settings);

    Highscores *m_highscores;
    AppSettings *m_settings;
};
