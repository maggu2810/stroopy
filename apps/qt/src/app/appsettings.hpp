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
    // Shown to other players (docs/spec/protocol/v1.md); 1-24 characters.
    Q_PROPERTY(QString playerName READ playerName WRITE setPlayerName NOTIFY playerNameChanged)
    // Nostr relay URLs, one per line; "" = the default relays.
    Q_PROPERTY(QString relays READ relays WRITE setRelays NOTIFY relaysChanged)
    Q_PROPERTY(QStringList defaultRelays READ defaultRelays CONSTANT)

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
    QString playerName() const { return m_playerName; }
    void setPlayerName(const QString &name);
    QString relays() const { return m_relays; }
    void setRelays(const QString &relays);
    QStringList defaultRelays() const;
    // The relays in use: the user's list, or the default relays if it is empty.
    QStringList effectiveRelays() const;
    // Removes control characters, trims and truncates to 24 code points ("" stays "").
    static QString cleanName(const QString &name);

signals:
    void languageChanged();
    void colourBlindChanged();
    void vibrationChanged();
    void playerNameChanged();
    void relaysChanged();

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
    QString m_playerName;
    QString m_relays;
};
