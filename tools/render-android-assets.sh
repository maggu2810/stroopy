#!/usr/bin/env bash
# Renders the SVG sources of the app's raster assets to PNGs. Requires inkscape.
# - Android TV banner (320x180)
# - Android adaptive launcher icon layers (foreground, monochrome) for all densities
# - Desktop window icon (256x256)
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/apps/qt/android/assets-src"
res="$root/apps/qt/android/res"

render() { # svg out width height
  mkdir -p "$(dirname "$2")"
  inkscape "$1" --export-text-to-path --export-type=png --export-width="$3" --export-height="$4" \
    --export-filename="$2" >/dev/null 2>&1
  echo "rendered $2"
}

render "$src/banner.svg" "$res/drawable/banner.png" 320 180

# Adaptive icon layers are 108 dp: mdpi 1x, hdpi 1.5x, xhdpi 2x, xxhdpi 3x, xxxhdpi 4x.
for density in mdpi:108 hdpi:162 xhdpi:216 xxhdpi:324 xxxhdpi:432; do
  name="${density%%:*}" size="${density##*:}"
  render "$src/icon-foreground.svg" "$res/mipmap-$name/ic_launcher_foreground.png" "$size" "$size"
  render "$src/icon-monochrome.svg" "$res/mipmap-$name/ic_launcher_monochrome.png" "$size" "$size"
done

render "$src/icon-full.svg" "$root/apps/qt/src/app/icons/stroopy.png" 256 256
