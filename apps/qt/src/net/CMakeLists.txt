# SPDX-License-Identifier: GPL-3.0-or-later
# Networking for live multiplayer (docs/spec/multiplayer/v1.md): Nostr events, messages, relays.
find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

qt_add_library(stroopy_net STATIC
    nostr.hpp nostr.cpp
    messages.hpp messages.cpp
    relaypool.hpp relaypool.cpp
)
target_include_directories(stroopy_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stroopy_net PUBLIC Qt6::Core Qt6::WebSockets stroopy_core PRIVATE secp256k1)
