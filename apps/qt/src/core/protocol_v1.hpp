// SPDX-License-Identifier: GPL-3.0-or-later
// Protocol v1 rooms (docs/spec/protocol/v1.md §1): room ids, room codes and the Nostr room tag.
// Messages and events need JSON and secp256k1; they live in src/net.
#pragma once

#include "codec.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace stroopy::protocol::v1 {

inline constexpr int version = 1;
inline constexpr int kind = 25713; // Nostr event kind (§3.1)

using Error = codec::Error;

// Lowest 40 bits -> 8 Crockford characters.
std::string roomIdFromValue(std::uint64_t value);
std::string roomCode(std::string_view roomId);
std::string roomTag(std::string_view roomId);
// Room code in arbitrary text, or a bare id typed by a user -> room id.
std::expected<std::string, Error> parseRoom(std::string_view text);

} // namespace stroopy::protocol::v1
