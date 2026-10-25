// SPDX-License-Identifier: GPL-3.0-or-later
#include "game_v1.hpp"

#include <regex>

namespace stroopy::game::v1 {

namespace {

constexpr std::size_t shortRecord = 14;
constexpr std::size_t longRecord = 18;
constexpr Error invalid{.kind = Error::Kind::Invalid};

} // namespace

std::uint16_t crc16(std::string_view data) noexcept
{
    return codec::crc16({reinterpret_cast<const std::uint8_t *>(data.data()), data.size()});
}

std::vector<std::uint8_t> encodeRecord(const Descriptor &d)
{
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(version), static_cast<std::uint8_t>(d.rules),
                                  static_cast<std::uint8_t>(d.mode)};
    codec::putU64(out, d.seed);
    out.push_back(d.result ? 1 : 0);
    if (d.result) {
        codec::putU16(out, d.result->score);
        codec::putU16(out, d.result->reached);
    }
    codec::putU16(out, codec::crc16(out));
    return out;
}

std::string encodeText(const Descriptor &descriptor)
{
    const auto record = encodeRecord(descriptor);
    return "SYG" + std::to_string(version) + "-" + codec::grouped(codec::base32(std::span{record}.subspan(1)));
}

std::expected<Descriptor, Error> decodeText(std::string_view text)
{
    static const std::regex pattern{R"(SYG([0-9]+)-([0-9A-Za-z]+(?:-[0-9A-Za-z]+)*))",
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
        const char c = codec::normalise(raw);
        if (c == '\0')
            return std::unexpected{invalid};
        body += c;
    }

    const std::size_t recordSize = body.size() == 21 ? shortRecord : body.size() == 28 ? longRecord : 0;
    if (recordSize == 0)
        return std::unexpected{invalid};
    auto decoded = codec::unbase32(body, recordSize - 1);
    if (!decoded)
        return std::unexpected{invalid};

    std::vector<std::uint8_t> record{static_cast<std::uint8_t>(version)};
    record.insert(record.end(), decoded->begin(), decoded->end());
    if (codec::crc16(std::span{record}.first(record.size() - 2)) != codec::getU16(record, record.size() - 2))
        return std::unexpected{invalid};
    const auto flags = record[11];
    if ((flags & ~1) != 0 || ((flags & 1) != 0) != (record.size() == longRecord))
        return std::unexpected{invalid};

    Descriptor d{.rules = record[1], .mode = record[2], .seed = codec::getU64(record, 3), .result = std::nullopt};
    if (flags & 1)
        d.result = Result{.score = codec::getU16(record, 12), .reached = codec::getU16(record, 14)};
    return d;
}

} // namespace stroopy::game::v1
