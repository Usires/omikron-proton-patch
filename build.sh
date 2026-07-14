#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

i686-w64-mingw32-gcc -shared \
  -Wl,--image-base=0x10000000 \
  -Wl,--no-seh \
  -Wl,--exclude-libs=ALL \
  -Wl,-u,_DirectDrawCreate@12 \
  -Wl,-u,_DirectDrawEnumerateA@8 \
  -Wl,--kill-at \
  -o patch.dll \
  dummy_patch.c \
  -lgcc

echo
echo "=== Section VMA ==="
objdump -h patch.dll 2>/dev/null | grep "CONTENTS" | head -3

echo
echo "=== UCRT-Check ==="
if objdump -p patch.dll 2>/dev/null | grep -q "api-ms-win-crt"; then
  echo "WARN: UCRT noch da"
else
  echo "OK: keine UCRT"
fi

echo
echo "=== Export Directory ==="
objdump -p patch.dll 2>/dev/null | grep -A 1 "Data Directory" | head -2

echo
echo "=== .text size ==="
objdump -h patch.dll 2>/dev/null | grep " \.text" | head -1

echo
echo "=== Datei: $(stat -c '%s bytes' patch.dll) ==="

GAME_DIR="$HOME/Games/Omikron"
if [ ! -d "$GAME_DIR" ]; then
  echo "FEHLER: $GAME_DIR existiert nicht"
  exit 1
fi

if [ ! -f "$GAME_DIR/patch.dll.original" ]; then
  mv "$GAME_DIR/patch.dll" "$GAME_DIR/patch.dll.original" 2>/dev/null || true
fi
cp patch.dll "$GAME_DIR/patch.dll"
echo "FERTIG."
