// SPDX-License-Identifier: GPL-3.0-or-later
// Building blocks shared by the text codes: Crockford Base32 and CRC-16/CCITT-FALSE
// (docs/spec/game/v1.md §1-2), and the parsing of the long codes that may be wrapped
// (backup v1 §2.1, multiplayer v1 §1.2).
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
    enum class Kind : std::uint8_t { Invalid, Unsupported, NoTransport };
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

// Valid UTF-8; with `noControl`, also no U+0000-U+001F or U+007F.
bool validUtf8(std::span<const std::uint8_t> s, bool noControl);

// The characters of a long code after its prefix: skips '-' and white space, normalises, and stops at
// the first character that is not in the alphabet.
std::string collect(std::string_view text);
// All whole bytes held by `chars` (alphabet characters only).
std::vector<std::uint8_t> wholeBytes(std::string_view chars);
// A record of `length` bytes (version byte included, it is not in `chars`) needs the first
// ceil((length-1)*8/5) characters, and their padding bits must be zero.
bool paddingOk(std::string_view chars, std::size_t length);

// Bounds-checked reader over decoded bytes; any overrun marks the record as too short.
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
        return getU16(data, pos - 2);
    }
    std::span<const std::uint8_t> bytes(std::size_t n)
    {
        if (!has(n))
            return {};
        pos += n;
        return data.subspan(pos - n, n);
    }
};

} // namespace stroopy::codec
