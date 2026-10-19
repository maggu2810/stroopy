#!/usr/bin/env bash
# Runs the Qt UI test (apps/qt, tst_ui) inside a private headless KWin (virtual output, OpenGL),
# so Material effects render as on devices. Nothing appears on the user's desktop.
# Usage: tools/run-ui-test-headless.sh [screenshot-dir] [test function...]
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
test_bin="$root/apps/qt/build/linux/tests/tst_ui"
[[ -x "$test_bin" ]] || { echo "build first: cmake --build --preset linux (in apps/qt)" >&2; exit 1; }

shots="${1:-}"
shift || true
socket="stroopy-ui-test-$$"

env -u DISPLAY -u WAYLAND_DISPLAY kwin_wayland --virtual --no-lockscreen \
  --width 1600 --height 1000 --socket "$socket" >/dev/null 2>&1 &
kwin_pid=$!
trap 'kill "$kwin_pid" 2>/dev/null || true' EXIT

for _ in $(seq 50); do
  [[ -S "${XDG_RUNTIME_DIR}/$socket" ]] && break
  sleep 0.1
done

WAYLAND_DISPLAY="$socket" QT_QPA_PLATFORM=wayland STROOPY_SCREENSHOT_DIR="$shots" "$test_bin" "$@"
