// SPDX-License-Identifier: GPL-3.0-or-later
#include "appsettings.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>

using namespace Qt::StringLiterals;

namespace {
const QStringList languages{u"en"_s, u"de"_s};
const auto keyLanguage = u"language"_s;
const auto keyColourBlind = u"colourBlind"_s;
const auto keyVibration = u"vibration"_s;
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
