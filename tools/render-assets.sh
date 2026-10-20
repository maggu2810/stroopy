#!/usr/bin/env bash
# Renders the SVG artwork in assets/artwork/ to the files the apps ship. Requires inkscape.
# Qt app:
# - Android TV banner (320x180)
# - Android adaptive launcher icon layers (foreground, monochrome) for all densities
# - Desktop window icon (256x256)
# Web app:
# - SVG favicon (text converted to paths), PNG icons, maskable icon, Apple touch icon
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/assets/artwork"
res="$root/apps/qt/android/res"
web="$root/apps/web/public/icons"

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

mkdir -p "$web"
inkscape "$src/icon-full.svg" --export-text-to-path --export-plain-svg --export-filename="$web/icon.svg" >/dev/null 2>&1
echo "rendered $web/icon.svg"
render "$src/icon-full.svg" "$web/icon-192.png" 192 192
render "$src/icon-full.svg" "$web/icon-512.png" 512 512
render "$src/icon-maskable.svg" "$web/maskable-512.png" 512 512
render "$src/icon-maskable.svg" "$web/apple-touch-icon.png" 180 180
