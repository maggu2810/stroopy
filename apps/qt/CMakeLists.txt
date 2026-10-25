# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 4.0)

project(Stroopy VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 LinguistTools)
qt_standard_project_setup(REQUIRES 6.12
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES de
)

# Shared spec directory (conformance vectors) of the repository.
set(STROOPY_SPEC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/spec")

# libsecp256k1 (MIT) for Nostr event signatures: only the Schnorr signature module, static.
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
foreach(module ECDH RECOVERY MUSIG ELLSWIFT SILENTPAYMENTS)
    set(SECP256K1_ENABLE_MODULE_${module} OFF CACHE BOOL "" FORCE)
endforeach()
set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "" FORCE)
foreach(option BUILD_BENCHMARK BUILD_TESTS BUILD_EXHAUSTIVE_TESTS BUILD_CTIME_TESTS BUILD_EXAMPLES INSTALL)
    set(SECP256K1_${option} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(secp256k1
    URL https://github.com/bitcoin-core/secp256k1/archive/refs/tags/v0.8.0.tar.gz
    URL_HASH SHA256=eb52b0e9239dff7dc26be5f9623567141b8720ec47da29eb3c1e0a660d17c8bb
    SYSTEM EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(secp256k1)

add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/app)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
