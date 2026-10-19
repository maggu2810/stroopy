// SPDX-License-Identifier: GPL-3.0-or-later
#include "share_code_v1.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

namespace stroopy::share_code::v1 {

namespace {

constexpr std::string_view alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
constexpr std::size_t shortRecord = 14;
constexpr std::size_t longRecord = 18;

std::uint16_t crc16(const std::uint8_t *data, std::size_t size) noexcept
{
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < size; ++i) {
        crc = static_cast<std::uint16_t>(crc ^ (data[i] << 8));
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<std::uint16_t>(crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1);
    }
    return crc;
}

void putU16(std::vector<std::uint8_t> &out, int value)
{
    const auto v = static_cast<std::uint16_t>(std::clamp(value, 0, 0xFFFF));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

int getU16(const std::vector<std::uint8_t> &in, std::size_t offset)
{
    return (in[offset] << 8) | in[offset + 1];
}

std::string base32(const std::uint8_t *data, std::size_t size)
{
    std::string out;
    std::uint32_t buffer = 0;
    int bits = 0;
    for (std::size_t i = 0; i < size; ++i) {
        buffer = (buffer << 8) | data[i];
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

// Decodes exactly `size` bytes; fails on bad length or non-zero padding bits.
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

constexpr Error invalid{.kind = Error::Kind::Invalid};

} // namespace

std::uint16_t crc16(std::string_view data) noexcept
{
    return crc16(reinterpret_cast<const std::uint8_t *>(data.data()), data.size());
}

std::vector<std::uint8_t> encodeRecord(const Descriptor &d)
{
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(version), static_cast<std::uint8_t>(d.rules),
                                  static_cast<std::uint8_t>(d.mode)};
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((d.seed >> shift) & 0xFF));
    out.push_back(d.result ? 1 : 0);
    if (d.result) {
        putU16(out, d.result->score);
        putU16(out, d.result->reached);
    }
    const auto crc = crc16(out.data(), out.size());
    out.push_back(static_cast<std::uint8_t>(crc >> 8));
    out.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    return out;
}

std::string encodeText(const Descriptor &descriptor)
{
    const auto record = encodeRecord(descriptor);
    const auto body = base32(record.data() + 1, record.size() - 1);
    std::string text = "SY" + std::to_string(version) + "-";
    for (std::size_t i = 0; i < body.size(); i += 4) {
        if (i > 0)
            text += '-';
        text += body.substr(i, 4);
    }
    return text;
}

std::expected<Descriptor, Error> decodeText(std::string_view text)
{
    static const std::regex pattern{R"(SY([0-9]+)-([0-9A-Za-z]+(?:-[0-9A-Za-z]+)*))",
                                    std::regex::ECMAScript | std::regex::icase};
    std::match_results<std::string_view::const_iterator> match;
    if (!std::regex_search(text.begin(), text.end(), match, pattern))
        return std::unexpected{invalid};

    const auto versionDigits = match[1].str();
    if (versionDigits.size() > 3 || std::stoi(versionDigits) != version)
        return std::unexpected{Error{.kind = Error::Kind::Unsupported,
                                     .version = versionDigits.size() > 3 ? -1 : std::stoi(versionDigits)}};

    std::string body;
    for (const char raw : match[2].str()) {
        if (raw == '-')
            continue;
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
        c = c == 'O' ? '0' : (c == 'I' || c == 'L') ? '1' : c;
        if (alphabet.find(c) == std::string_view::npos)
            return std::unexpected{invalid};
        body += c;
    }

    const std::size_t recordSize = body.size() == 21 ? shortRecord : body.size() == 28 ? longRecord : 0;
    if (recordSize == 0)
        return std::unexpected{invalid};
    auto decoded = unbase32(body, recordSize - 1);
    if (!decoded)
        return std::unexpected{invalid};

    std::vector<std::uint8_t> record{static_cast<std::uint8_t>(version)};
    record.insert(record.end(), decoded->begin(), decoded->end());
    if (crc16(record.data(), record.size() - 2) != getU16(record, record.size() - 2))
        return std::unexpected{invalid};
    const auto flags = record[11];
    if ((flags & ~1) != 0 || ((flags & 1) != 0) != (record.size() == longRecord))
        return std::unexpected{invalid};

    Descriptor d{.rules = record[1], .mode = record[2], .seed = 0, .result = std::nullopt};
    for (std::size_t i = 3; i < 11; ++i)
        d.seed = (d.seed << 8) | record[i];
    if (flags & 1)
        d.result = Result{.score = getU16(record, 12), .reached = getU16(record, 14)};
    return d;
}

} // namespace stroopy::share_code::v1
