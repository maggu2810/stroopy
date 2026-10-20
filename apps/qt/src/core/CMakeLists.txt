# SPDX-License-Identifier: GPL-3.0-or-later
# Game core: plain C++23, no Qt dependency.
add_library(stroopy_core STATIC
    codec.hpp codec.cpp
    rules_v1.hpp rules_v1.cpp
    game.hpp game.cpp
    share_code_v1.hpp share_code_v1.cpp
    backup_v1.hpp backup_v1.cpp
    protocol_v1.hpp protocol_v1.cpp
)
target_include_directories(stroopy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(stroopy_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Werror>
)
