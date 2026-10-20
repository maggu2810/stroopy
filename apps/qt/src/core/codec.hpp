// SPDX-License-Identifier: GPL-3.0-or-later
// Building blocks shared by the text codes: Crockford Base32 and CRC-16/CCITT-FALSE
// (docs/spec/share-code/v1.md §1-2).
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace stroopy::codec {

inline constexpr std::string_view alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

struct Error {
    enum class Kind : std::uint8_t { Invalid, Unsupported };
    Kind kind;
    int version = 0; // version found, for Unsupported
};

std::uint16_t crc16(std::span<const std::uint8_t> data) noexcept;

// Big-endian bit string in 5-bit groups, last group padded with zero bits.
std::string base32(std::span<const std::uint8_t> data);
// Decodes exactly `size` bytes; fails on bad length or non-zero padding bits.
std::optional<std::vector<std::uint8_t>> unbase32(std::string_view text, std::size_t size);

// Upper case plus the aliases O->0, I->1, L->1; '\0' if the result is not in the alphabet.
char normalise(char c) noexcept;
// Groups of 4 characters joined by '-'.
std::string grouped(std::string_view text);

void putU16(std::vector<std::uint8_t> &out, std::int64_t value); // clamped to 0..65535
void putU64(std::vector<std::uint8_t> &out, std::uint64_t value);
int getU16(std::span<const std::uint8_t> in, std::size_t offset);
std::uint64_t getU64(std::span<const std::uint8_t> in, std::size_t offset);

} // namespace stroopy::codec
