// SPDX-License-Identifier: GPL-3.0-or-later
// Multiplayer v1 invitations (docs/spec/multiplayer/v1.md §1): room ids, the SYM text code with the
// room's relays, and the Nostr room tag. Messages and events need JSON and secp256k1; they live in src/net.
#pragma once

#include "codec.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace stroopy::multiplayer::v1 {

inline constexpr int version = 1;
inline constexpr int kind = 25713; // Nostr event kind (§3.1)
inline constexpr std::size_t maxRelays = 8;

using Error = codec::Error;

struct Invitation {
    std::string room; // room id as 8 Crockford characters
    std::vector<std::string> relays;
    bool operator==(const Invitation &) const = default;
};

// Lowest 40 bits -> 8 Crockford characters.
std::string roomIdFromValue(std::uint64_t value);
std::string roomTag(std::string_view roomId);
// A valid relay entry (§1.1): ws(s) scheme, no white space or control characters, at most 255 bytes.
bool isRelayUrl(std::string_view url);

std::vector<std::uint8_t> encodeRecord(const Invitation &invitation);
std::string encodeText(const Invitation &invitation);
// Finds and decodes the first invitation in arbitrary text (§1.2). Without a Nostr relay:
// Error::Kind::NoTransport.
std::expected<Invitation, Error> decodeText(std::string_view text);

} // namespace stroopy::multiplayer::v1
