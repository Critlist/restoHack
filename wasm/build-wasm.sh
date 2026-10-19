#!/bin/sh
# wasm/build-wasm.sh - build the browser bundle.
#
# The generic build steps (pinned emsdk check, checksum-verified xterm
# assets, CMake configure/build) live in the unix-tui-to-wasm template
# submodule at wasm/utw. This wrapper points that script at the restoHack
# tree and keeps the historical build directories below build/.
#
# Environment overrides (WEB_DIR, OBJ_DIR, ALLOW_UNTESTED_EMSDK, ...) are
# passed through unchanged.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEMPLATE="$ROOT/wasm/utw/wasm/build-wasm.sh"

if [ ! -f "$TEMPLATE" ]; then
  echo "Fetching the unix-tui-to-wasm submodule..." >&2
  if ! git -C "$ROOT" submodule update --init wasm/utw; then
    echo "error: the unix-tui-to-wasm submodule is missing; run:" >&2
    echo "  git submodule update --init wasm/utw" >&2
    exit 1
  fi
fi

: "${WEB_DIR:=$ROOT/build/wasm}"       # the servable bundle
: "${OBJ_DIR:=$ROOT/build/wasm-build}" # CMake working dir
export WEB_DIR OBJ_DIR
ROOT="$ROOT" exec "$TEMPLATE" "$@"
