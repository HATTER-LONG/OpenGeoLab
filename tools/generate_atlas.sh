#!/bin/bash
# Generate MSDF font atlas for label rendering.
# Requires: msdf-atlas-gen (https://github.com/Chlumsky/msdf-atlas-gen)
# Usage: ./tools/generate_atlas.sh /path/to/font.ttf

FONT="${1:?Usage: $0 <font.ttf>}"
OUT_DIR="src/libs/render/resource/fonts"

msdf-atlas-gen \
  -font "$FONT" \
  -charset ascii \
  -type msdf \
  -dimensions 512 512 \
  -pxrange 4 \
  -json "${OUT_DIR}/label_atlas.json" \
  -imageout "${OUT_DIR}/label_atlas.png"

echo "Atlas generated in ${OUT_DIR}/"
