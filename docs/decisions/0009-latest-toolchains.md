# 0009: Always use the latest toolchains and language standards

Status: accepted (2026-10-19)

## Context
The project is new and has no users on old platforms. Supporting old
compilers or SDKs would only cost time.

## Decision
Use the newest stable releases and features, with no compatibility with old
versions:
- Qt: latest release (currently **6.12.0**), installed via `tools/setup-qt.sh`
  (aqtinstall).
- CMake: latest (currently 4.x). `cmake_minimum_required` and
  `qt_standard_project_setup(REQUIRES …)` are set to the versions we
  actually use.
- C++: **C++23** (newer once compilers support it). Prefer modern idioms
  (`std::expected`, ranges, `std::format`, modules once stable in our
  toolchain).
- QML: modern Qt 6 style: `qt_add_qml_module`, declarative type registration
  (`QML_ELEMENT`), `pragma ComponentBehavior: Bound`, qmllint-clean, and
  compiled QML via qmlsc / qmltc where useful.
- Android: latest SDK platform and NDK supported by the Qt release in use.
  `minSdk` as high as practical.
- Web: latest Bun, TypeScript and Preact; current browsers only.
- Other stacks (Kotlin/Compose, Swift/SwiftUI) follow the same rule when
  they start.

## Consequences
- Contributors need current toolchains. Setup scripts in `tools/` pin the
  exact versions.
- Bumping toolchains is routine work and gets its own commits.
