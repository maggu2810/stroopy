// SPDX-License-Identifier: GPL-3.0-or-later
#include "codec.hpp"

#include <algorithm>
#include <cctype>

namespace stroopy::codec {

std::uint16_t crc16(std::span<const std::uint8_t> data) noexcept
{
    std::uint16_t crc = 0xFFFF;
    for (const auto byte : data) {
        crc = static_cast<std::uint16_t>(crc ^ (byte << 8));
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<std::uint16_t>(crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1);
    }
    return crc;
}

std::string base32(std::span<const std::uint8_t> data)
{
    std::string out;
    std::uint32_t buffer = 0;
    int bits = 0;
    for (const auto byte : data) {
        buffer = (buffer << 8) | byte;
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            out += alphabet[(buffer >> bits) & 31];
        }
    }
    if (bits > 0)
        out += alphabet[(buffer << (5 - bits)) & 31];
    return out;
}

std::optional<std::vector<std::uint8_t>> unbase32(std::string_view text, std::size_t size)
{
    if (text.size() * 5 < size * 8 || text.size() * 5 - size * 8 >= 5)
        return std::nullopt;
    std::vector<std::uint8_t> out;
    std::uint32_t buffer = 0;
    int bits = 0;
    for (const char c : text) {
        buffer = (buffer << 5) | static_cast<std::uint32_t>(alphabet.find(c));
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFF));
        }
    }
    if ((buffer & ((1U << bits) - 1)) != 0)
        return std::nullopt;
    return out;
}

char normalise(char raw) noexcept
{
    auto c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    c = c == 'O' ? '0' : (c == 'I' || c == 'L') ? '1' : c;
    return alphabet.find(c) == std::string_view::npos ? '\0' : c;
}

std::string grouped(std::string_view text)
{
    std::string out;
    for (std::size_t i = 0; i < text.size(); i += 4) {
        if (i > 0)
            out += '-';
        out += text.substr(i, 4);
    }
    return out;
}

void putU16(std::vector<std::uint8_t> &out, std::int64_t value)
{
    const auto v = static_cast<std::uint16_t>(std::clamp<std::int64_t>(value, 0, 0xFFFF));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void putU64(std::vector<std::uint8_t> &out, std::uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
}

int getU16(std::span<const std::uint8_t> in, std::size_t offset)
{
    return (in[offset] << 8) | in[offset + 1];
}

std::uint64_t getU64(std::span<const std::uint8_t> in, std::size_t offset)
{
    std::uint64_t v = 0;
    for (std::size_t i = 0; i < 8; ++i)
        v = (v << 8) | in[offset + i];
    return v;
}

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

std::string collect(std::string_view text)
{
    std::string chars;
    for (const char raw : text) {
        if (raw == '-' || std::isspace(static_cast<unsigned char>(raw)))
            continue;
        const char c = normalise(raw);
        if (c == '\0')
            break;
        chars += c;
    }
    return chars;
}

std::vector<std::uint8_t> wholeBytes(std::string_view chars)
{
    std::vector<std::uint8_t> out;
    std::uint32_t buffer = 0;
    int bits = 0;
    for (const char c : chars) {
        buffer = (buffer << 5) | static_cast<std::uint32_t>(alphabet.find(c));
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFF));
        }
    }
    return out;
}

bool paddingOk(std::string_view chars, std::size_t length)
{
    const auto needed = ((length - 1) * 8 + 4) / 5;
    if (needed > chars.size())
        return false;
    const auto pad = needed * 5 - (length - 1) * 8;
    return (alphabet.find(chars[needed - 1]) & ((1U << pad) - 1)) == 0;
}

} // namespace stroopy::codec
