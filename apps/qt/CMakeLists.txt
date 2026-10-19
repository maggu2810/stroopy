# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 4.0)

project(Stroopy VERSION 0.1.0 LANGUAGES CXX)

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

add_subdirectory(src/core)
add_subdirectory(src/app)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
