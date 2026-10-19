#!/usr/bin/env bash
# Installs the Qt kits used by apps/qt via aqtinstall (no Qt account needed).
# Usage: tools/setup-qt.sh            (installs into $QT_ROOT, default ~/Qt-aqt)
# Requires: pipx (installs aqtinstall if missing).
set -euo pipefail

QT_VERSION="${QT_VERSION:-6.12.0}"
QT_ROOT="${QT_ROOT:-$HOME/Qt-aqt}"
MODULES=(qtconnectivity qtwebsockets qtmultimedia qtshadertools)

command -v aqt >/dev/null || pipx install aqtinstall

# Desktop kit (also the host kit required for Android cross builds).
aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 \
  -O "$QT_ROOT" -m "${MODULES[@]}"
# Android kit for arm64 phones.
aqt install-qt all_os android "$QT_VERSION" android_arm64_v8a \
  -O "$QT_ROOT" -m "${MODULES[@]}"

echo "Qt $QT_VERSION installed in $QT_ROOT"
