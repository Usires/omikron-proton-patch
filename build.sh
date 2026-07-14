#!/bin/bash
#
# build.sh — compile forwarder.c -> patch.dll and (optionally) install.
#
# Default mode (no arguments): build, validate, then install into
#   $GAME_DIR (default: ~/Games/Omikron).
#
# CI mode (--ci or BUILD_ONLY=1): build and validate only. No install,
#   no filesystem writes outside the working directory. Use this from
#   GitHub Actions or any sandbox where you don't want side effects.
#
# Usage:
#   ./build.sh             # build + install
#   ./build.sh --ci        # build only (for CI)
#   BUILD_ONLY=1 ./build.sh  # same as --ci
#   GAME_DIR=/path/to/game ./build.sh   # custom install location

set -euo pipefail

# --- Mode selection ----------------------------------------------------

BUILD_ONLY=0
for arg in "$@"; do
  case "$arg" in
    --ci) BUILD_ONLY=1 ;;
    -h|--help)
      sed -n '2,18p' "$0"
      exit 0
      ;;
    *) echo "Unknown argument: $arg" >&2; exit 2 ;;
  esac
done
if [ "${BUILD_ONLY:-0}" = "1" ]; then
  BUILD_ONLY=1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# --- Build ------------------------------------------------------------

echo "=== Compiling src/forwarder.c ==="
i686-w64-mingw32-gcc -shared \
  -Wl,--image-base=0x10000000 \
  -Wl,--no-seh \
  -Wl,--exclude-libs=ALL \
  -Wl,-u,_DirectDrawCreate@12 \
  -Wl,-u,_DirectDrawEnumerateA@8 \
  -Wl,--kill-at \
  -o patch.dll \
  src/forwarder.c \
  -lgcc

# --- Validate ---------------------------------------------------------

echo
echo "=== File ==="
ls -la patch.dll
file patch.dll || true

echo
echo "=== Exports (must include DirectDrawCreate + DirectDrawEnumerateA) ==="
EXPORTS=$(i686-w64-mingw32-objdump -p patch.dll | grep -E "^\s+[0-9]+\s+[0-9a-f]+\s+(DirectDraw|DllMain)" || true)
echo "$EXPORTS"
if ! echo "$EXPORTS" | grep -q DirectDrawCreate; then
  echo "FAIL: DirectDrawCreate not exported" >&2
  exit 1
fi
if ! echo "$EXPORTS" | grep -q DirectDrawEnumerateA; then
  echo "FAIL: DirectDrawEnumerateA not exported" >&2
  exit 1
fi

echo
echo "=== Imports (should be ONLY kernel32.dll) ==="
i686-w64-mingw32-objdump -p patch.dll | grep -E "^[[:space:]]*DLL Name" || true

echo
echo "=== UCRT check (must be empty) ==="
if i686-w64-mingw32-objdump -p patch.dll | grep -q "api-ms-win-crt"; then
  echo "FAIL: UCRT dependency found" >&2
  exit 1
fi
echo "OK: no UCRT dependencies"

echo
echo "=== Build OK ==="

# --- Install (skip in CI mode) ----------------------------------------

if [ "$BUILD_ONLY" = "1" ]; then
  echo "BUILD_ONLY=1 — skipping install step"
  exit 0
fi

GAME_DIR="${GAME_DIR:-$HOME/Games/Omikron}"

if [ ! -d "$GAME_DIR" ]; then
  echo "FEHLER: $GAME_DIR existiert nicht" >&2
  echo "        Setze GAME_DIR=/pfad/zum/spiel oder erstelle das Verzeichnis." >&2
  exit 1
fi

if [ ! -f "$GAME_DIR/patch.dll.original" ]; then
  # Only back up if the existing file is NOT already our build (sanity check).
  if i686-w64-mingw32-objdump -p "$GAME_DIR/patch.dll" 2>/dev/null \
       | grep -q "src/forwarder.c"; then
    echo "patch.dll in $GAME_DIR is already our build — no backup needed."
  else
    mv "$GAME_DIR/patch.dll" "$GAME_DIR/patch.dll.original"
    echo "Backed up original patch.dll -> patch.dll.original"
  fi
fi
cp patch.dll "$GAME_DIR/patch.dll"
echo "FERTIG."
