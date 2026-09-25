#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

PORT="$(require_port)"
cd "$ROOT"

exec pio device monitor --port "$PORT" --baud 115200
