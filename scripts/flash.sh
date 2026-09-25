#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

command -v pio >/dev/null 2>&1 ||
  fail "PlatformIO missing. Run: brew install platformio"

PORT="$(require_port)"
cd "$ROOT"

info "Building + flashing through $PORT"
pio run -t upload --upload-port "$PORT"

ok "Flash complete"
printf 'Serial log if you want it:\n  bash scripts/monitor.sh\n'
