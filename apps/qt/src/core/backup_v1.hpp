// SPDX-License-Identifier: GPL-3.0-or-later
// Backup code v1 (docs/spec/backup/v1.md): highscores and shared settings <-> record bytes <-> text code.
#pragma once

#include "codec.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stroopy::backup::v1 {

inline constexpr int version = 1;
inline constexpr std::size_t maxNameBytes = 64;

using Error = codec::Error;

struct Entry {
    int mode;
    int rules;
    std::uint64_t seed;
    int score;
    int reached;
    std::int64_t time; // Unix seconds, UTC
    bool challenge;
    bool operator==(const Entry &) const = default;
};

// Shared settings; std::nullopt = not in the backup (or skipped on decoding).
struct Settings {
    std::optional<std::string> language; // "" = follow the system
    std::optional<bool> colourBlind;
    std::optional<bool> vibration;
    std::optional<std::string> name;
    std::optional<std::string> relays; // '\n'-separated, "" = default relays
    bool operator==(const Settings &) const = default;
};

struct Backup {
    std::vector<Entry> entries;
    std::optional<Settings> settings; // std::nullopt = no settings section
    bool operator==(const Backup &) const = default;
};

std::vector<std::uint8_t> encodeRecord(const Backup &backup);
std::string encodeText(const Backup &backup);
// Finds and decodes the first backup code in arbitrary text (spec §2.1).
std::expected<Backup, Error> decodeText(std::string_view text);

} // namespace stroopy::backup::v1
