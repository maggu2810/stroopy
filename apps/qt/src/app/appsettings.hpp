// SPDX-License-Identifier: GPL-3.0-or-later
// Persistent user settings (docs/design/product.md, "Settings") and runtime language switching.
#pragma once

#include <QObject>
#include <QSettings>
#include <QTranslator>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class AppSettings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // "" = follow system, otherwise a language code from supportedLanguages.
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // Language actually in use (resolved from the system when following it).
    Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList supportedLanguages READ supportedLanguages CONSTANT)
    Q_PROPERTY(bool colourBlind READ colourBlind WRITE setColourBlind NOTIFY colourBlindChanged)
    Q_PROPERTY(bool vibration READ vibration WRITE setVibration NOTIFY vibrationChanged)

public:
    static AppSettings *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString effectiveLanguage() const { return m_effectiveLanguage; }
    QStringList supportedLanguages() const;
    bool colourBlind() const { return m_colourBlind; }
    void setColourBlind(bool on);
    bool vibration() const { return m_vibration; }
    void setVibration(bool on);

signals:
    void languageChanged();
    void colourBlindChanged();
    void vibrationChanged();

private:
    explicit AppSettings(QQmlEngine *engine);
    void applyLanguage();

    QQmlEngine *m_engine;
    QSettings m_settings;
    QTranslator m_translator;
    QString m_language;
    QString m_effectiveLanguage;
    bool m_colourBlind = false;
    bool m_vibration = true;
};
