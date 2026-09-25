#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

[[ -n "$(find_esptool)" ]] ||
  fail "esptool missing. Run: brew install esptool"

PORT="$(require_port)"
STAMP="$(date +%Y%m%d-%H%M%S)"
DIR="$ROOT/backups/heltec-v3-$STAMP"
mkdir -p "$DIR"

info "Reading device information from $PORT"
run_esptool_action "$PORT" flash-id flash_id | tee "$DIR/device-info.txt"

info "Backing up the entire flash (Meshtastic + NVS/config included)"
run_esptool_action "$PORT" read-flash read_flash 0 ALL "$DIR/flash.bin"

(
  cd "$DIR"
  shasum -a 256 flash.bin | tee sha256.txt
)

printf '%s\n' "$PORT" > "$DIR/port.txt"
ln -sfn "$(basename "$DIR")" "$ROOT/backups/latest"

ok "Backup complete: $DIR"
printf 'Restore later with:\n  bash scripts/restore.sh %q\n' "$DIR"
