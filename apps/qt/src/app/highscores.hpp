// SPDX-License-Identifier: GPL-3.0-or-later
// Local highscores per mode (docs/design/product.md, "Highscores"), stored as JSON.
#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class Highscores : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Increments on every change; lets QML bindings that call entries()/best() re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    static constexpr int maxEntriesPerMode = 10;

    explicit Highscores(QObject *parent = nullptr);

    // Entries of one mode, best first: {score, reached, rules, mode, seedHex, time, challenge}.
    Q_INVOKABLE QVariantList entries(int mode) const;
    Q_INVOKABLE int best(int mode) const;
    // Stores the game if it makes the top list; returns its 1-based rank or 0.
    Q_INVOKABLE int submit(int rules, int mode, const QString &seedHex, int score, int reached, bool challenge);
    Q_INVOKABLE void clear();
    int revision() const { return m_revision; }

signals:
    void changed();

private:
    void load();
    void save() const;

    QVariantList m_entries;
    int m_revision = 0;
};
