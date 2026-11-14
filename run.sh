#!/usr/bin/env bash
# run.sh - Build (if needed) and run NexusForge IDE

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${NEXUS_BUILD_DIR:-$ROOT_DIR/out/build}"

BIN_EXT=""
case "$(uname -s 2>/dev/null || echo Unknown)" in
    MINGW*|MSYS*|CYGWIN*) BIN_EXT=".exe" ;;
esac
BIN_PATH="$BUILD_DIR/bin/NexusForge${BIN_EXT}"

if [[ ! -x "$BIN_PATH" ]]; then
    exec bash "$ROOT_DIR/build.sh" --build-dir "$BUILD_DIR" -- "$@"
fi

exec "$BIN_PATH" "$@"
