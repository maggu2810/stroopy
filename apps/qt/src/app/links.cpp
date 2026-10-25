// SPDX-License-Identifier: GPL-3.0-or-later
#include "links.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QQmlEngine>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

namespace {

Links *s_instance = nullptr;

} // namespace

Links::Links(QObject *parent) : QObject{parent}
{
    s_instance = this;
    QDesktopServices::setUrlHandler(u"stroopy"_s, this, "handleUrl");
}

Links::~Links()
{
    QDesktopServices::unsetUrlHandler(u"stroopy"_s);
    s_instance = nullptr;
}

Links *Links::create(QQmlEngine *, QJSEngine *)
{
    auto *links = instance();
    QQmlEngine::setObjectOwnership(links, QQmlEngine::CppOwnership);
    return links;
}

Links *Links::instance()
{
    if (!s_instance)
        new Links{QCoreApplication::instance()};
    return s_instance;
}

void Links::handleUrl(const QUrl &url)
{
    // Only the c parameter counts (game §4, multiplayer §1.4); the pages decode it.
    const auto code = QUrlQuery{url}.queryItemValue(u"c"_s, QUrl::FullyDecoded);
    if (code.isEmpty())
        return;
    if (url.host() == u"play"_s)
        emit play(code);
    else if (url.host() == u"join"_s)
        emit join(code);
}
