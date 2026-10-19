#!/usr/bin/env bash
# Renders SVG sources of Android-only assets (TV banner) to PNGs. Requires inkscape.
set -euo pipefail
dir="$(cd "$(dirname "$0")/../apps/qt/android" && pwd)"
inkscape "$dir/assets-src/banner.svg" --export-type=png --export-width=320 --export-height=180 \
  --export-filename="$dir/res/drawable/banner.png"
echo "rendered $dir/res/drawable/banner.png"
