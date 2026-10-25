// SPDX-License-Identifier: GPL-3.0-or-later
#include "backup_v1.hpp"

#include <algorithm>
#include <iterator>
#include <regex>

namespace stroopy::backup::v1 {

namespace {

constexpr std::size_t entrySize = 19;
constexpr Error invalid{.kind = Error::Kind::Invalid};

enum Key : std::uint8_t { Language = 1, ColourBlind = 2, Vibration = 3, Name = 4, Relays = 5 };

void putItem(std::vector<std::uint8_t> &out, Key key, std::string_view value)
{
    out.push_back(key);
    codec::putU16(out, static_cast<std::int64_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

} // namespace

std::vector<std::uint8_t> encodeRecord(const Backup &backup)
{
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(version), backup.settings ? std::uint8_t{1} : std::uint8_t{0}};
    codec::putU16(out, static_cast<std::int64_t>(backup.entries.size()));
    for (const auto &e : backup.entries) {
        out.push_back(static_cast<std::uint8_t>(e.mode));
        out.push_back(static_cast<std::uint8_t>(e.rules));
        codec::putU64(out, e.seed);
        codec::putU16(out, e.score);
        codec::putU16(out, e.reached);
        const auto time = static_cast<std::uint32_t>(std::clamp<std::int64_t>(e.time, 0, 0xFFFFFFFF));
        for (int shift = 24; shift >= 0; shift -= 8)
            out.push_back(static_cast<std::uint8_t>((time >> shift) & 0xFF));
        out.push_back(e.challenge ? 1 : 0);
    }
    if (const auto &s = backup.settings) {
        const auto count = static_cast<std::uint8_t>(s->language.has_value() + s->colourBlind.has_value()
                                                     + s->vibration.has_value() + s->name.has_value()
                                                     + s->relays.has_value());
        out.push_back(count);
        if (s->language)
            putItem(out, Language, *s->language);
        if (s->colourBlind)
            putItem(out, ColourBlind, *s->colourBlind ? "\x01" : std::string_view{"\x00", 1});
        if (s->vibration)
            putItem(out, Vibration, *s->vibration ? "\x01" : std::string_view{"\x00", 1});
        if (s->name)
            putItem(out, Name, std::string_view{*s->name}.substr(0, maxNameBytes));
        if (s->relays)
            putItem(out, Relays, *s->relays);
    }
    codec::putU16(out, codec::crc16(out));
    return out;
}

std::string encodeText(const Backup &backup)
{
    const auto record = encodeRecord(backup);
    return "SYB" + std::to_string(version) + "-" + codec::grouped(codec::base32(std::span{record}.subspan(1)));
}

std::expected<Backup, Error> decodeText(std::string_view text)
{
    static const std::regex prefix{R"(SYB([0-9]+)-)", std::regex::ECMAScript | std::regex::icase};
    std::match_results<std::string_view::const_iterator> match;
    if (!std::regex_search(text.begin(), text.end(), match, prefix))
        return std::unexpected{invalid};
    const auto versionDigits = match[1].str();
    if (versionDigits.size() > 3 || std::stoi(versionDigits) != version)
        return std::unexpected{Error{.kind = Error::Kind::Unsupported,
                                     .version = versionDigits.size() > 3 ? -1 : std::stoi(versionDigits)}};

    // Collect characters (§2.1 step 3), then decode all whole bytes they hold.
    const auto chars = codec::collect({match[0].second, text.end()});
    std::vector<std::uint8_t> data{static_cast<std::uint8_t>(version)};
    std::ranges::copy(codec::wholeBytes(chars), std::back_inserter(data));
    codec::Reader r{.data = data, .pos = 1};
    const auto flags = r.u8();
    const auto count = static_cast<std::size_t>(r.u16());
    if (!r.overrun && (flags & ~1) != 0)
        return std::unexpected{invalid};
    Backup backup;
    for (std::size_t i = 0; i < count && !r.overrun; ++i) {
        const auto e = r.bytes(entrySize);
        if (r.overrun)
            break;
        if ((e[18] & ~1) != 0)
            return std::unexpected{invalid};
        const std::int64_t time = (std::int64_t{e[14]} << 24) | (e[15] << 16) | (e[16] << 8) | e[17];
        backup.entries.push_back({.mode = e[0], .rules = e[1], .seed = codec::getU64(e, 2),
                                  .score = codec::getU16(e, 10), .reached = codec::getU16(e, 12), .time = time,
                                  .challenge = (e[18] & 1) != 0});
    }
    if (!r.overrun && (flags & 1) != 0) {
        Settings s;
        std::vector<int> seen;
        const auto items = r.u8();
        for (int i = 0; i < items && !r.overrun; ++i) {
            const auto key = r.u8();
            const auto len = static_cast<std::size_t>(r.u16());
            const auto value = r.bytes(len);
            if (r.overrun)
                break;
            if (std::ranges::contains(seen, key) || key >= 128)
                return std::unexpected{invalid};
            seen.push_back(key);
            const std::string str{value.begin(), value.end()};
            switch (key) {
            case ColourBlind:
            case Vibration:
                if (len != 1 || value[0] > 1)
                    return std::unexpected{invalid};
                (key == ColourBlind ? s.colourBlind : s.vibration) = value[0] == 1;
                break;
            case Language:
            case Relays:
                if (!codec::validUtf8(value, false))
                    return std::unexpected{invalid};
                (key == Language ? s.language : s.relays) = str;
                break;
            case Name:
                if (len > maxNameBytes || !codec::validUtf8(value, true))
                    return std::unexpected{invalid};
                s.name = str;
                break;
            default:
                break; // unknown key: skipped
            }
        }
        backup.settings = s;
    }
    const auto crcPos = r.pos;
    r.u16();
    if (r.overrun)
        return std::unexpected{invalid};

    // Exactly the characters needed for the record belong to the code (§2.1 step 4).
    if (!codec::paddingOk(chars, crcPos + 2))
        return std::unexpected{invalid};
    if (codec::crc16(std::span{data}.first(crcPos)) != codec::getU16(data, crcPos))
        return std::unexpected{invalid};
    return backup;
}

} // namespace stroopy::backup::v1
