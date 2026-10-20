// SPDX-License-Identifier: GPL-3.0-or-later
#include "backup_v1.hpp"

#include <algorithm>
#include <cctype>
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

// Valid UTF-8; with `noControl`, also no U+0000-U+001F or U+007F.
bool validUtf8(std::span<const std::uint8_t> s, bool noControl)
{
    for (std::size_t i = 0; i < s.size();) {
        const auto b = s[i];
        std::size_t len = b < 0x80 ? 1 : (b >> 5) == 0x6 ? 2 : (b >> 4) == 0xE ? 3 : (b >> 3) == 0x1E ? 4 : 0;
        if (len == 0 || i + len > s.size())
            return false;
        std::uint32_t cp = len == 1 ? b : b & (0x7Fu >> len);
        for (std::size_t k = 1; k < len; ++k) {
            if ((s[i + k] & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (s[i + k] & 0x3Fu);
        }
        constexpr std::uint32_t minimum[] = {0, 0, 0x80, 0x800, 0x10000};
        if (cp < minimum[len] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        if (noControl && (cp < 0x20 || cp == 0x7F))
            return false;
        i += len;
    }
    return true;
}

// Bounds-checked reader over the decoded bytes; any overrun marks the record as too short.
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t pos = 0;
    bool overrun = false;

    bool has(std::size_t n)
    {
        if (pos + n > data.size())
            overrun = true;
        return !overrun;
    }
    std::uint8_t u8() { return has(1) ? data[pos++] : 0; }
    int u16()
    {
        if (!has(2))
            return 0;
        pos += 2;
        return codec::getU16(data, pos - 2);
    }
    std::span<const std::uint8_t> bytes(std::size_t n)
    {
        if (!has(n))
            return {};
        pos += n;
        return data.subspan(pos - n, n);
    }
};

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
    std::string chars;
    for (auto it = match[0].second; it != text.end(); ++it) {
        if (*it == '-' || std::isspace(static_cast<unsigned char>(*it)))
            continue;
        const char c = codec::normalise(*it);
        if (c == '\0')
            break;
        chars += c;
    }
    std::vector<std::uint8_t> data{static_cast<std::uint8_t>(version)};
    {
        std::uint32_t buffer = 0;
        int bits = 0;
        for (const char c : chars) {
            buffer = (buffer << 5) | static_cast<std::uint32_t>(codec::alphabet.find(c));
            bits += 5;
            if (bits >= 8) {
                bits -= 8;
                data.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFF));
            }
        }
    }

    Reader r{.data = data, .pos = 1};
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
                if (!validUtf8(value, false))
                    return std::unexpected{invalid};
                (key == Language ? s.language : s.relays) = str;
                break;
            case Name:
                if (len > maxNameBytes || !validUtf8(value, true))
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
    const auto length = crcPos + 2;
    const auto needed = ((length - 1) * 8 + 4) / 5;
    if (needed > chars.size())
        return std::unexpected{invalid};
    const auto pad = needed * 5 - (length - 1) * 8;
    if ((codec::alphabet.find(chars[needed - 1]) & ((1U << pad) - 1)) != 0)
        return std::unexpected{invalid};
    if (codec::crc16(std::span{data}.first(crcPos)) != codec::getU16(data, crcPos))
        return std::unexpected{invalid};
    return backup;
}

} // namespace stroopy::backup::v1
