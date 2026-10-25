// SPDX-License-Identifier: GPL-3.0-or-later
#include "multiplayer_v1.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <regex>

namespace stroopy::multiplayer::v1 {

namespace {

constexpr Error invalid{.kind = Error::Kind::Invalid};
constexpr std::uint8_t tagNostr = 1;

} // namespace

std::string roomIdFromValue(std::uint64_t value)
{
    std::string id;
    for (int i = 0; i < 8; ++i)
        id += codec::alphabet[(value >> (35 - 5 * i)) & 31];
    return id;
}

std::string roomTag(std::string_view roomId)
{
    std::string tag = "stroopy-m" + std::to_string(version) + "-";
    std::ranges::transform(roomId, std::back_inserter(tag),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return tag;
}

bool isRelayUrl(std::string_view url)
{
    const auto scheme = url.starts_with("wss://") ? 6U : url.starts_with("ws://") ? 5U : 0U;
    if (scheme == 0 || url.size() <= scheme || url.size() > 255)
        return false;
    const auto *bytes = reinterpret_cast<const std::uint8_t *>(url.data());
    if (!codec::validUtf8({bytes, url.size()}, false))
        return false;
    return std::ranges::none_of(url, [](char c) {
        const auto u = static_cast<unsigned char>(c);
        return u <= 0x20 || u == 0x7F;
    });
}

std::vector<std::uint8_t> encodeRecord(const Invitation &invitation)
{
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(version)};
    std::ranges::copy(codec::wholeBytes(invitation.room), std::back_inserter(out));
    out.push_back(static_cast<std::uint8_t>(invitation.relays.size()));
    for (const auto &url : invitation.relays) {
        out.push_back(tagNostr);
        out.push_back(static_cast<std::uint8_t>(url.size()));
        out.insert(out.end(), url.begin(), url.end());
    }
    codec::putU16(out, codec::crc16(out));
    return out;
}

std::string encodeText(const Invitation &invitation)
{
    const auto record = encodeRecord(invitation);
    return "SYM" + std::to_string(version) + "-" + codec::grouped(codec::base32(std::span{record}.subspan(1)));
}

std::expected<Invitation, Error> decodeText(std::string_view text)
{
    static const std::regex prefix{R"(SYM([0-9]+)-)", std::regex::ECMAScript | std::regex::icase};
    std::match_results<std::string_view::const_iterator> match;
    if (!std::regex_search(text.begin(), text.end(), match, prefix))
        return std::unexpected{invalid};
    const auto versionDigits = match[1].str();
    if (versionDigits.size() > 3 || std::stoi(versionDigits) != version)
        return std::unexpected{Error{.kind = Error::Kind::Unsupported,
                                     .version = versionDigits.size() > 3 ? -1 : std::stoi(versionDigits)}};

    const auto chars = codec::collect({match[0].second, text.end()});
    std::vector<std::uint8_t> data{static_cast<std::uint8_t>(version)};
    std::ranges::copy(codec::wholeBytes(chars), std::back_inserter(data));
    codec::Reader r{.data = data, .pos = 1};
    const auto room = r.bytes(5);
    const auto count = r.u8();
    if (!r.overrun && count == 0)
        return std::unexpected{invalid};
    Invitation invitation{.room = codec::base32(room), .relays = {}};
    for (int i = 0; i < count && !r.overrun; ++i) {
        const auto tag = r.u8();
        const auto value = r.bytes(r.u8());
        if (r.overrun)
            break;
        if (tag >= 128)
            return std::unexpected{invalid};
        if (tag != tagNostr)
            continue; // unknown transport: skipped
        const std::string url{value.begin(), value.end()};
        if (!isRelayUrl(url) || std::ranges::contains(invitation.relays, url)
            || invitation.relays.size() == maxRelays)
            return std::unexpected{invalid};
        invitation.relays.push_back(url);
    }
    const auto crcPos = r.pos;
    r.u16();
    if (r.overrun || !codec::paddingOk(chars, crcPos + 2)
        || codec::crc16(std::span{data}.first(crcPos)) != codec::getU16(data, crcPos))
        return std::unexpected{invalid};
    if (invitation.relays.empty())
        return std::unexpected{Error{.kind = Error::Kind::NoTransport}};
    return invitation;
}

} // namespace stroopy::multiplayer::v1
