// SPDX-License-Identifier: GPL-3.0-or-later
#include "protocol_v1.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <regex>

namespace stroopy::protocol::v1 {

namespace {

constexpr Error invalid{.kind = Error::Kind::Invalid};

std::optional<std::string> normaliseId(std::string_view chars)
{
    std::string id;
    for (const char raw : chars) {
        const char c = codec::normalise(raw);
        if (c == '\0')
            return std::nullopt;
        id += c;
    }
    return id;
}

} // namespace

std::string roomIdFromValue(std::uint64_t value)
{
    std::string id;
    for (int i = 0; i < 8; ++i)
        id += codec::alphabet[(value >> (35 - 5 * i)) & 31];
    return id;
}

std::string roomCode(std::string_view roomId)
{
    return "SYR" + std::to_string(version) + "-" + std::string{roomId.substr(0, 4)} + "-"
           + std::string{roomId.substr(4)};
}

std::string roomTag(std::string_view roomId)
{
    std::string tag = "stroopy-p" + std::to_string(version) + "-";
    std::ranges::transform(roomId, std::back_inserter(tag),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return tag;
}

std::expected<std::string, Error> parseRoom(std::string_view text)
{
    static const std::regex code{R"(SYR([0-9]+)-([0-9A-Za-z]{4})-?([0-9A-Za-z]{4}))",
                                 std::regex::ECMAScript | std::regex::icase};
    static const std::regex bare{R"(^([0-9A-Za-z]{4})-?([0-9A-Za-z]{4})$)", std::regex::ECMAScript};

    std::match_results<std::string_view::const_iterator> m;
    if (std::regex_search(text.begin(), text.end(), m, code)) {
        const auto digits = m[1].str();
        if (digits.size() > 3 || std::stoi(digits) != version)
            return std::unexpected{Error{.kind = Error::Kind::Unsupported,
                                         .version = digits.size() > 3 ? -1 : std::stoi(digits)}};
        if (auto id = normaliseId(m[2].str() + m[3].str()))
            return *id;
        return std::unexpected{invalid};
    }
    const auto first = text.find_first_not_of(" \t\r\n");
    const auto last = text.find_last_not_of(" \t\r\n");
    const auto trimmed = first == std::string_view::npos ? std::string_view{} : text.substr(first, last - first + 1);
    if (std::regex_match(trimmed.begin(), trimmed.end(), m, bare))
        if (auto id = normaliseId(m[1].str() + m[2].str()))
            return *id;
    return std::unexpected{invalid};
}

} // namespace stroopy::protocol::v1
