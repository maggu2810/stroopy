// SPDX-License-Identifier: GPL-3.0-or-later
#include "nostr.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QRandomGenerator>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

using namespace Qt::StringLiterals;

namespace stroopy::net {

namespace {

const secp256k1_context *context()
{
    static secp256k1_context *ctx = [] {
        auto *c = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        QByteArray seed(32, Qt::Uninitialized);
        QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(seed.data()), 8);
        // Side-channel hardening only; a failure is harmless.
        [[maybe_unused]] const int randomized =
            secp256k1_context_randomize(c, reinterpret_cast<const unsigned char *>(seed.constData()));
        return c;
    }();
    return ctx;
}

const unsigned char *bytes(const QByteArray &b)
{
    return reinterpret_cast<const unsigned char *>(b.constData());
}

QByteArray randomBytes(int n)
{
    QByteArray out(n, Qt::Uninitialized);
    for (int i = 0; i < n; ++i)
        out[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    return out;
}

bool isLowerHex(const QString &s, qsizetype length)
{
    return s.size() == length
           && std::ranges::all_of(s, [](QChar c) { return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f'); });
}

} // namespace

QString jsonString(const QString &s)
{
    QString out = u"\""_s;
    for (const QChar c : s) {
        switch (c.unicode()) {
        case '"': out += u"\\\""_s; break;
        case '\\': out += u"\\\\"_s; break;
        case '\n': out += u"\\n"_s; break;
        case '\r': out += u"\\r"_s; break;
        case '\t': out += u"\\t"_s; break;
        case '\b': out += u"\\b"_s; break;
        case '\f': out += u"\\f"_s; break;
        default: out += c;
        }
    }
    return out + u'"';
}

QByteArray serialise(const QString &pubkey, qint64 createdAt, int kind, const QList<QStringList> &tags,
                     const QString &content)
{
    QStringList tagList;
    for (const auto &tag : tags) {
        QStringList items;
        for (const auto &item : tag)
            items.append(jsonString(item));
        tagList.append(u'[' + items.join(u',') + u']');
    }
    return u"[0,%1,%2,%3,[%4],%5]"_s.arg(jsonString(pubkey)).arg(createdAt).arg(kind).arg(tagList.join(u','),
                                                                                        jsonString(content))
        .toUtf8();
}

QJsonObject Event::toJson() const
{
    QJsonArray tagArray;
    for (const auto &tag : tags)
        tagArray.append(QJsonArray::fromStringList(tag));
    return {{u"id"_s, id},          {u"pubkey"_s, pubkey}, {u"created_at"_s, createdAt}, {u"kind"_s, kind},
            {u"tags"_s, tagArray}, {u"content"_s, content}, {u"sig"_s, sig}};
}

std::optional<Event> Event::fromJson(const QJsonValue &value)
{
    if (!value.isObject())
        return std::nullopt;
    const auto o = value.toObject();
    for (const auto &key : {u"id"_s, u"pubkey"_s, u"content"_s, u"sig"_s})
        if (!o[key].isString())
            return std::nullopt;
    if (!o[u"created_at"_s].isDouble() || !o[u"kind"_s].isDouble() || !o[u"tags"_s].isArray())
        return std::nullopt;
    Event e{.id = o[u"id"_s].toString(), .pubkey = o[u"pubkey"_s].toString(),
            .createdAt = o[u"created_at"_s].toInteger(), .kind = o[u"kind"_s].toInt(), .tags = {},
            .content = o[u"content"_s].toString(), .sig = o[u"sig"_s].toString()};
    for (const auto &tag : o[u"tags"_s].toArray()) {
        if (!tag.isArray())
            return std::nullopt;
        QStringList items;
        for (const auto &item : tag.toArray()) {
            if (!item.isString())
                return std::nullopt;
            items.append(item.toString());
        }
        e.tags.append(items);
    }
    return e;
}

SecretKey SecretKey::generate()
{
    for (;;) {
        if (auto key = fromBytes(randomBytes(32)))
            return *key;
    }
}

std::optional<SecretKey> SecretKey::fromBytes(const QByteArray &b)
{
    if (b.size() != 32 || !secp256k1_ec_seckey_verify(context(), bytes(b)))
        return std::nullopt;
    return SecretKey{b};
}

QString SecretKey::publicKeyHex() const
{
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pub;
    QByteArray out(32, Qt::Uninitialized);
    // The key is valid by construction (fromBytes), so these cannot fail.
    [[maybe_unused]] const int created = secp256k1_keypair_create(context(), &keypair, bytes(m_bytes));
    [[maybe_unused]] const int derived = secp256k1_keypair_xonly_pub(context(), &pub, nullptr, &keypair);
    secp256k1_xonly_pubkey_serialize(context(), reinterpret_cast<unsigned char *>(out.data()), &pub);
    return QString::fromLatin1(out.toHex());
}

Event SecretKey::sign(qint64 createdAt, int kind, const QList<QStringList> &tags, const QString &content,
                      const QByteArray &aux) const
{
    Event e{.id = {}, .pubkey = publicKeyHex(), .createdAt = createdAt, .kind = kind, .tags = tags,
            .content = content, .sig = {}};
    const auto id = QCryptographicHash::hash(serialise(e.pubkey, createdAt, kind, tags, content),
                                             QCryptographicHash::Sha256);
    secp256k1_keypair keypair;
    [[maybe_unused]] const int created = secp256k1_keypair_create(context(), &keypair, bytes(m_bytes));
    QByteArray sig(64, Qt::Uninitialized);
    const auto auxRand = aux.size() == 32 ? aux : randomBytes(32);
    [[maybe_unused]] const int signedOk = secp256k1_schnorrsig_sign32(context(), reinterpret_cast<unsigned char *>(sig.data()), bytes(id), &keypair,
                                      bytes(auxRand));
    e.id = QString::fromLatin1(id.toHex());
    e.sig = QString::fromLatin1(sig.toHex());
    return e;
}

bool verify(const Event &e)
{
    if (!isLowerHex(e.id, 64) || !isLowerHex(e.pubkey, 64) || !isLowerHex(e.sig, 128))
        return false;
    const auto id = QCryptographicHash::hash(serialise(e.pubkey, e.createdAt, e.kind, e.tags, e.content),
                                             QCryptographicHash::Sha256);
    if (QString::fromLatin1(id.toHex()) != e.id)
        return false;
    secp256k1_xonly_pubkey pub;
    if (!secp256k1_xonly_pubkey_parse(context(), &pub, bytes(QByteArray::fromHex(e.pubkey.toLatin1()))))
        return false;
    const auto sig = QByteArray::fromHex(e.sig.toLatin1());
    return secp256k1_schnorrsig_verify(context(), bytes(sig), bytes(id), 32, &pub) == 1;
}

} // namespace stroopy::net
