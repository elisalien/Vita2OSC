#!/bin/bash
set -euo pipefail

export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"

rm -rf "$VITASDK"
mkdir -p "$VITASDK"

URL=$(wget -qO- https://github.com/vitasdk/vita-headers/raw/master/.travis.d/last_built_toolchain.py | python3 - master win)
echo "Download URL: $URL"

TMP=/tmp/vitasdk-toolchain.tar.bz2
wget -O "$TMP" "$URL"
echo "Downloaded $(stat -c%s "$TMP" 2>/dev/null || wc -c < "$TMP") bytes"
bzip2 -t "$TMP"
tar xj -C "$VITASDK" --strip-components=1 -f "$TMP"
rm -f "$TMP"

echo "VitaSDK installed to $VITASDK"
ls "$VITASDK/share/vita.toolchain.cmake"
ls "$VITASDK/bin/arm-vita-eabi-gcc"
