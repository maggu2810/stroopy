#!/usr/bin/env bash
# Downloads the original "True Color" APK (com.aurelhubert.truecolor) and
# decompiles it into third-party/com.aurelhubert.truecolor/ for study only.
# Requires: curl, jadx, apktool.
set -euo pipefail

pkg="com.aurelhubert.truecolor"
expected_sha256="9d5ff814d4897d023f5a59bac6d3028b11ec89f8d55180a313d0b821b7a68c09" # v3.0.2
root="$(cd "$(dirname "$0")/.." && pwd)"
dir="$root/third-party/$pkg"
apk="$dir/apk/truecolor-3.0.2.apk"

mkdir -p "$dir/apk" "$dir/decompiled"
if [[ ! -f "$apk" ]]; then
  curl -fSL -A "Mozilla/5.0 (X11; Linux x86_64)" -o "$apk" \
    "https://d.apkpure.com/b/APK/$pkg?version=latest"
fi
echo "$expected_sha256  $apk" | sha256sum -c -

jadx -q -d "$dir/decompiled/jadx" "$apk"
apktool d -q -f -o "$dir/decompiled/apktool" "$apk"
echo "Done: $dir/decompiled"
