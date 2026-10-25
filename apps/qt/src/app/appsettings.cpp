// SPDX-License-Identifier: GPL-3.0-or-later
#include "appsettings.hpp"

#include "multiplayer_v1.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlEngine>
#include <QRandomGenerator>

#include <optional>
#include <utility>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

using namespace Qt::StringLiterals;

namespace {
const QStringList languages{u"en"_s, u"de"_s};
const auto keyLanguage = u"language"_s;
const auto keyColourBlind = u"colourBlind"_s;
const auto keyVibration = u"vibration"_s;
const auto keyPlayerName = u"playerName"_s;
const auto keyRelays = u"relays"_s;
constexpr std::size_t maxNameLength = 24;

#ifdef Q_OS_ANDROID
// Per-app language of Android 13+ (LocaleManager), also set in the system settings.
QJniObject localeManager()
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 33)
        return {};
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    return context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
                                    QJniObject::fromString(u"locale"_s).object());
}
#endif

// The system's language for this app ("" = follow system), or nothing if the OS has none.
std::optional<QString> systemAppLanguage()
{
#ifdef Q_OS_ANDROID
    if (const auto manager = localeManager(); manager.isValid()) {
        const auto locales = manager.callObjectMethod("getApplicationLocales", "()Landroid/os/LocaleList;");
        if (locales.isValid()) {
            const auto tags = locales.callObjectMethod("toLanguageTags", "()Ljava/lang/String;").toString();
            return tags.section(u',', 0, 0).section(u'-', 0, 0).toLower();
        }
    }
#endif
    return std::nullopt;
}

void setSystemAppLanguage([[maybe_unused]] const QString &language)
{
#ifdef Q_OS_ANDROID
    if (const auto manager = localeManager(); manager.isValid()) {
        const auto locales = QJniObject::callStaticObjectMethod(
            "android/os/LocaleList", "forLanguageTags", "(Ljava/lang/String;)Landroid/os/LocaleList;",
            QJniObject::fromString(language).object());
        manager.callMethod<void>("setApplicationLocales", "(Landroid/os/LocaleList;)V", locales.object());
    }
#endif
}
} // namespace

AppSettings *AppSettings::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    return new AppSettings{qmlEngine};
}

AppSettings::AppSettings(QQmlEngine *engine)
    : m_engine{engine},
      m_language{m_settings.value(keyLanguage).toString()},
      m_colourBlind{m_settings.value(keyColourBlind, false).toBool()},
      m_vibration{m_settings.value(keyVibration, true).toBool()}
{
    if (!languages.contains(m_language))
        m_language.clear();
    // The system's per-app language wins, also when the user changes it there while we run.
    if (const auto system = systemAppLanguage()) {
        m_language = languages.contains(*system) ? *system : QString{};
        m_settings.setValue(keyLanguage, m_language);
        connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive)
                if (const auto system = systemAppLanguage())
                    setLanguage(*system);
        });
    }
    m_relays = m_settings.value(keyRelays).toString();
    m_playerName = cleanName(m_settings.value(keyPlayerName).toString());
    if (m_playerName.isEmpty()) {
        // Chosen once, so the name stays the same until the user changes it.
        m_playerName = u"Player%1"_s.arg(QRandomGenerator::global()->bounded(100, 1000));
        m_settings.setValue(keyPlayerName, m_playerName);
    }
    applyLanguage();
}

QStringList AppSettings::supportedLanguages() const
{
    return languages;
}

void AppSettings::setLanguage(const QString &language)
{
    const auto value = languages.contains(language) ? language : QString{};
    if (value == m_language)
        return;
    m_language = value;
    m_settings.setValue(keyLanguage, m_language);
    if (const auto system = systemAppLanguage(); system && *system != value)
        setSystemAppLanguage(value);
    applyLanguage();
}

void AppSettings::applyLanguage()
{
    QString lang = m_language;
    if (lang.isEmpty()) {
        lang = u"en"_s;
        for (const auto &ui : QLocale::system().uiLanguages(QLocale::TagSeparator::Dash)) {
            if (const auto code = ui.section(u'-', 0, 0).toLower(); languages.contains(code)) {
                lang = code;
                break;
            }
        }
    }
    QCoreApplication::removeTranslator(&m_translator);
    // Source strings are English; other languages come from compiled .qm resources.
    if (lang != u"en"_s && m_translator.load(u":/i18n/Stroopy_"_s + lang))
        QCoreApplication::installTranslator(&m_translator);
    m_effectiveLanguage = lang;
    if (m_engine)
        m_engine->retranslate();
    emit languageChanged();
}

void AppSettings::setColourBlind(bool on)
{
    if (on == m_colourBlind)
        return;
    m_colourBlind = on;
    m_settings.setValue(keyColourBlind, on);
    emit colourBlindChanged();
}

void AppSettings::setVibration(bool on)
{
    if (on == m_vibration)
        return;
    m_vibration = on;
    m_settings.setValue(keyVibration, on);
    emit vibrationChanged();
}

QString AppSettings::cleanName(const QString &name)
{
    std::u32string out;
    for (const auto c : name.toStdU32String())
        if (c >= 0x20 && c != 0x7F)
            out += c;
    out = QString::fromStdU32String(out).trimmed().toStdU32String();
    return QString::fromStdU32String(out.substr(0, maxNameLength));
}

void AppSettings::setPlayerName(const QString &name)
{
    const auto value = cleanName(name);
    if (value.isEmpty() || value == m_playerName)
        return;
    m_playerName = value;
    m_settings.setValue(keyPlayerName, value);
    emit playerNameChanged();
}

void AppSettings::setRelays(const QString &relays)
{
    QStringList urls;
    for (const auto &line : relays.split(u'\n'))
        if (const auto url = line.trimmed(); !url.isEmpty())
            urls.append(url);
    const auto value = urls.join(u'\n');
    if (value == m_relays)
        return;
    m_relays = value;
    m_settings.setValue(keyRelays, value);
    emit relaysChanged();
}

QStringList AppSettings::defaultRelays() const
{
    // docs/design/nostr-relays.md, list of 2026-10-25 (multiplayer v1 §3.4)
    return {u"wss://relay.damus.io"_s, u"wss://nos.lol"_s, u"wss://relay.primal.net"_s};
}

QStringList AppSettings::roomRelays() const
{
    namespace mp = stroopy::multiplayer::v1;
    QStringList out;
    for (const auto &url : m_relays.split(u'\n'))
        if (mp::isRelayUrl(url.toStdString()) && !out.contains(url) && std::cmp_less(out.size(), mp::maxRelays))
            out.append(url);
    return out.isEmpty() ? defaultRelays() : out;
}
