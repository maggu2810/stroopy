// SPDX-License-Identifier: GPL-3.0-or-later
#include "multiplayer.hpp"

#include "appsettings.hpp"
#include "protocol_v1.hpp"
#include "relaypool.hpp"
#include "rules_v1.hpp"
#include "share_code_v1.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QQmlEngine>
#include <QRandomGenerator>

using namespace Qt::StringLiterals;
namespace net = stroopy::net;
namespace pv = stroopy::protocol::v1;

namespace {

constexpr int heartbeatMs = 5000;
constexpr int minStatusIntervalMs = 500;
constexpr int presenceTimeoutMs = 15000;
const QList<int> supportedRules{stroopy::rules::v1::version};

qint64 now()
{
    return QDateTime::currentMSecsSinceEpoch();
}

std::optional<net::Start> decodeGame(const QString &code)
{
    const auto d = stroopy::share_code::v1::decodeText(code.toStdString());
    if (!d || d->result)
        return std::nullopt;
    return net::Start{.code = code, .rules = d->rules, .mode = d->mode, .seed = d->seed};
}

} // namespace

Multiplayer *Multiplayer::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    return new Multiplayer{qmlEngine->singletonInstance<AppSettings *>("Stroopy", "AppSettings")};
}

Multiplayer::Multiplayer(AppSettings *settings) : m_settings{settings}
{
    m_heartbeat.setInterval(1000);
    connect(&m_heartbeat, &QTimer::timeout, this, &Multiplayer::tick);
    m_statusTimer.setSingleShot(true);
    connect(&m_statusTimer, &QTimer::timeout, this, [this] { sendStatus(true); });
    connect(settings, &AppSettings::playerNameChanged, this, [this] {
        m_me.status.name = m_settings->playerName();
        if (active())
            sendStatus();
    });
}

Multiplayer::~Multiplayer()
{
    close();
}

QVariantMap Multiplayer::parseRoom(const QString &text)
{
    const auto r = pv::parseRoom(text.toStdString());
    if (r)
        return {{u"ok"_s, true}};
    return {{u"ok"_s, false},
            {u"error"_s, r.error().kind == pv::Error::Kind::Unsupported ? u"unsupported"_s : u"invalid"_s}};
}

void Multiplayer::createRoom()
{
    open(pv::roomIdFromValue(QRandomGenerator::system()->generate64()), true);
}

bool Multiplayer::join(const QString &text)
{
    const auto r = pv::parseRoom(text.toStdString());
    if (!r)
        return false;
    open(*r, false);
    return true;
}

void Multiplayer::open(const std::string &roomId, bool asHost)
{
    close();
    m_roomId = roomId;
    m_secret = net::SecretKey::generate();
    m_me = Player{.key = m_secret->publicKeyHex(),
                  .status = {.name = m_settings->playerName(), .host = asHost, .rules = supportedRules,
                             .mode = std::nullopt, .game = std::nullopt},
                  .lastSeen = now()};
    m_players.clear();
    m_departed.clear();
    m_played.clear();
    m_pending.reset();
    m_mode = static_cast<int>(stroopy::rules::v1::Mode::Classic);
    m_lastSent = 0;
    m_lastHostSeen = now();
    const QJsonObject filter{{u"kinds"_s, QJsonArray{pv::kind}},
                             {u"#t"_s, QJsonArray{QString::fromStdString(pv::roomTag(roomId))}}};
    m_pool = new net::RelayPool{m_settings->effectiveRelays(), filter, this};
    connect(m_pool, &net::RelayPool::eventReceived, this, &Multiplayer::receive);
    connect(m_pool, &net::RelayPool::connectedChanged, this, [this] {
        // Announce as soon as a relay is up.
        if (m_pool && m_pool->connected() > 0)
            sendStatus(true);
        emit changed();
    });
    m_heartbeat.start();
    emit changed();
    emit opened();
}

void Multiplayer::leave()
{
    if (!active())
        return;
    send(net::Bye{});
    // Give the bye a moment to leave before the sockets close.
    auto *pool = m_pool;
    m_pool = nullptr;
    QTimer::singleShot(300, pool, &QObject::deleteLater);
    close();
}

void Multiplayer::close()
{
    m_heartbeat.stop();
    m_statusTimer.stop();
    delete m_pool;
    m_pool = nullptr;
    m_players.clear();
    m_pending.reset();
    emit changed();
}

QString Multiplayer::roomCode() const
{
    return active() ? QString::fromStdString(pv::roomCode(m_roomId)) : QString{};
}

int Multiplayer::connected() const
{
    return m_pool ? m_pool->connected() : 0;
}

int Multiplayer::relayCount() const
{
    return m_pool ? m_pool->total() : 0;
}

QList<const Multiplayer::Player *> Multiplayer::everyone() const
{
    QList<const Player *> out{&m_me};
    for (const auto &p : m_players)
        out.append(&p);
    return out;
}

QString Multiplayer::hostKey() const
{
    QString key;
    for (const auto *p : everyone())
        if (p->status.host && (key.isEmpty() || p->key < key))
            key = p->key;
    return key;
}

bool Multiplayer::isHost() const
{
    return active() && hostKey() == m_me.key;
}

QString Multiplayer::hostName() const
{
    const auto key = hostKey();
    for (const auto *p : everyone())
        if (p->key == key)
            return p->status.name;
    return {};
}

bool Multiplayer::canStart() const
{
    const bool everyoneCanPlay = std::ranges::all_of(m_players, [](const Player &p) {
        return p.status.rules.contains(stroopy::rules::v1::version);
    });
    return isHost() && connected() > 0 && !m_busy && everyoneCanPlay;
}

QVariantList Multiplayer::players() const
{
    QVariantList out;
    const auto host = hostKey();
    for (const auto *p : everyone())
        out.append(QVariantMap{{u"name"_s, p->status.name}, {u"me"_s, p == &m_me}, {u"host"_s, p->key == host}});
    return out;
}

QVariantList Multiplayer::ranking() const
{
    if (!m_me.status.game)
        return {};
    QList<const Player *> list;
    auto candidates = everyone();
    for (const auto &p : m_departed)
        candidates.append(&p);
    for (const auto *p : candidates)
        if (p->status.game && p->status.game->code == m_me.status.game->code)
            list.append(p);
    std::ranges::stable_sort(list, [](const Player *a, const Player *b) {
        return a->status.game->score != b->status.game->score ? a->status.game->score > b->status.game->score
                                                              : a->status.game->n > b->status.game->n;
    });
    QVariantList out;
    for (const auto *p : list)
        out.append(QVariantMap{{u"name"_s, p->status.name}, {u"me"_s, p == &m_me},
                               {u"score"_s, p->status.game->score}, {u"n"_s, p->status.game->n},
                               {u"over"_s, p->status.game->over}});
    return out;
}

int Multiplayer::lastMode() const
{
    const auto game = m_me.status.game ? decodeGame(m_me.status.game->code) : std::nullopt;
    return game ? game->mode : 0;
}

QVariantList Multiplayer::opponents() const
{
    QVariantList out;
    for (const auto &item : ranking())
        if (!item.toMap()[u"me"_s].toBool())
            out.append(item);
    return out;
}

QString Multiplayer::inviteText() const
{
    return tr("Play Stroopy with me: %1").arg(roomCode());
}

void Multiplayer::setMode(int mode)
{
    m_mode = mode;
    sendStatus();
    emit changed();
}

void Multiplayer::start()
{
    if (!canStart())
        return;
    const auto seed = QRandomGenerator::system()->generate64();
    const stroopy::share_code::v1::Descriptor d{.rules = stroopy::rules::v1::version, .mode = m_mode, .seed = seed,
                                                .result = std::nullopt};
    const net::Start game{.code = QString::fromStdString(stroopy::share_code::v1::encodeText(d)),
                          .rules = d.rules, .mode = d.mode, .seed = seed};
    send(game);
    begin(game);
}

void Multiplayer::joinPending()
{
    if (m_pending)
        begin(*m_pending);
}

void Multiplayer::begin(const net::Start &game)
{
    m_played.insert(game.code);
    m_pending.reset();
    m_me.status.game = net::GameState{.code = game.code, .n = 1, .score = 0, .over = false};
    m_busy = true;
    emit play(game.mode, u"%1"_s.arg(game.seed, 16, 16, QLatin1Char('0')).toUpper());
    sendStatus(true);
    emit changed();
}

void Multiplayer::offer(const net::Start &game)
{
    if (m_played.contains(game.code) || !supportedRules.contains(game.rules)
        || !stroopy::rules::v1::modeFromId(game.mode))
        return;
    if (m_busy) {
        m_pending = game;
        emit changed();
    } else {
        begin(game);
    }
}

void Multiplayer::report(int n, int score, bool over)
{
    if (!active() || !m_me.status.game)
        return;
    auto &g = *m_me.status.game;
    if (g.n == n && g.score == score && g.over == over)
        return;
    g = {.code = g.code, .n = n, .score = score, .over = over};
    sendStatus(over);
    emit changed();
}

void Multiplayer::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit changed();
}

void Multiplayer::sendStatus(bool urgent)
{
    if (!active())
        return;
    const auto wait = m_lastSent + minStatusIntervalMs - now();
    if (urgent || wait <= 0) {
        m_statusTimer.stop();
        auto status = m_me.status;
        status.mode = isHost() ? std::optional{m_mode} : std::nullopt;
        send(status);
    } else if (!m_statusTimer.isActive()) {
        m_statusTimer.start(static_cast<int>(wait));
    }
}

void Multiplayer::send(const net::Message &message)
{
    if (!m_pool || !m_secret)
        return;
    if (std::holds_alternative<net::Status>(message))
        m_lastSent = now();
    const QList<QStringList> tags{{u"t"_s, QString::fromStdString(pv::roomTag(m_roomId))}};
    m_pool->publish(m_secret->sign(QDateTime::currentSecsSinceEpoch(), pv::kind, tags, net::encodeMessage(message)));
}

void Multiplayer::tick()
{
    const auto t = now();
    if (t - m_lastSent >= heartbeatMs)
        sendStatus(true);
    removePlayers([t](const Player &p) { return t - p.lastSeen >= presenceTimeoutMs; });
    // No host for 15 s: the present player with the lowest key takes over (§4).
    if (!hostKey().isEmpty()) {
        m_lastHostSeen = t;
    } else if (t - m_lastHostSeen >= presenceTimeoutMs
               && std::ranges::none_of(m_players, [this](const Player &p) { return p.key < m_me.key; })) {
        m_me.status.host = true;
        sendStatus(true);
    }
}

void Multiplayer::removePlayers(const std::function<bool(const Player &)> &gone)
{
    bool removed = false;
    for (auto it = m_players.begin(); it != m_players.end();) {
        if (!gone(*it)) {
            ++it;
            continue;
        }
        m_departed.removeIf([&](const Player &d) { return d.key == it->key; });
        if (it->status.game)
            m_departed.append(*it);
        it = m_players.erase(it);
        removed = true;
    }
    if (removed)
        emit changed();
}

void Multiplayer::receive(const net::Event &event)
{
    if (event.pubkey == m_me.key)
        return;
    const auto message = net::parseMessage(event.content);
    if (!message)
        return;
    const auto known = std::ranges::find(m_players, event.pubkey, &Player::key);

    if (std::holds_alternative<net::Bye>(*message)) {
        removePlayers([&](const Player &p) { return p.key == event.pubkey; });
        return;
    }
    if (const auto *start = std::get_if<net::Start>(&*message)) {
        if (known != m_players.end())
            known->lastSeen = now();
        if (event.pubkey == hostKey())
            offer(*start);
        return;
    }

    const auto &status = std::get<net::Status>(*message);
    const bool newcomer = known == m_players.end();
    if (!newcomer) {
        known->status = status;
        known->lastSeen = now();
    } else {
        m_players.append(Player{.key = event.pubkey, .status = status, .lastSeen = now()});
    }
    // A lower-keyed host wins (§4): stop claiming.
    if (m_me.status.host && !isHost())
        m_me.status.host = false;
    if (event.pubkey == hostKey()) {
        if (status.mode && !isHost())
            m_mode = *status.mode;
        // Late join: the host is playing a game we haven't played (§4).
        if (status.game && !status.game->over)
            if (const auto game = decodeGame(status.game->code))
                offer(*game);
    }
    if (newcomer)
        sendStatus(); // let the newcomer know us quickly
    emit changed();
}
