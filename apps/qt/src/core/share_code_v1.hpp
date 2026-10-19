// SPDX-License-Identifier: GPL-3.0-or-later
// Share code v1 (docs/spec/share-code/v1.md): game descriptor <-> record bytes <-> text code.
#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stroopy::share_code::v1 {

inline constexpr int version = 1;

struct Result {
    int score;
    int reached;
    bool operator==(const Result &) const = default;
};

struct Descriptor {
    int rules;
    int mode;
    std::uint64_t seed;
    std::optional<Result> result;
    bool operator==(const Descriptor &) const = default;
};

struct Error {
    enum class Kind : std::uint8_t { Invalid, Unsupported };
    Kind kind;
    int version = 0; // share-code version found, for Unsupported
};

std::uint16_t crc16(std::string_view data) noexcept;

std::vector<std::uint8_t> encodeRecord(const Descriptor &descriptor);
std::string encodeText(const Descriptor &descriptor);
// Finds and decodes the first share code in arbitrary text (spec §2.1).
std::expected<Descriptor, Error> decodeText(std::string_view text);

} // namespace stroopy::share_code::v1
