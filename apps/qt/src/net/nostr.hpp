// SPDX-License-Identifier: GPL-3.0-or-later
// Nostr events (NIP-01) as used by docs/spec/protocol/v1.md §3: serialisation, id, BIP-340 signatures.
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace stroopy::net {

struct Event {
    QString id;
    QString pubkey;
    qint64 createdAt = 0;
    int kind = 0;
    QList<QStringList> tags;
    QString content;
    QString sig;

    QJsonObject toJson() const;
    // Structural parsing only; use verify() for id and signature.
    static std::optional<Event> fromJson(const QJsonValue &value);
    bool operator==(const Event &) const = default;
};

// JSON string per NIP-01: only " \ \n \r \t \b \f are escaped, everything else is verbatim.
QString jsonString(const QString &s);
// UTF-8 of [0,pubkey,created_at,kind,tags,content]
QByteArray serialise(const QString &pubkey, qint64 createdAt, int kind, const QList<QStringList> &tags,
                     const QString &content);

class SecretKey {
public:
    // A fresh random key.
    static SecretKey generate();
    // 32 bytes; std::nullopt if not a valid secp256k1 secret key.
    static std::optional<SecretKey> fromBytes(const QByteArray &bytes);

    QString publicKeyHex() const;
    // Signed event. `aux`: 32 bytes of BIP-340 auxiliary randomness (random if empty; fixed for tests).
    Event sign(qint64 createdAt, int kind, const QList<QStringList> &tags, const QString &content,
               const QByteArray &aux = {}) const;

private:
    explicit SecretKey(QByteArray bytes) : m_bytes{std::move(bytes)} {}
    QByteArray m_bytes;
};

// True if the id matches the content and the signature verifies (§3.3).
bool verify(const Event &event);

} // namespace stroopy::net
