#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

command -v pio >/dev/null 2>&1 ||
  fail "PlatformIO missing. Run: brew install platformio"

cd "$ROOT"
info "Building Heltec V3 firmware..."
pio run
ok "Build complete"
