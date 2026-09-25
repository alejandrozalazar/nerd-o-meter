#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

[[ -n "$(find_esptool)" ]] ||
  fail "esptool missing. Run: brew install esptool"

DIR="${1:-$ROOT/backups/latest}"
[[ -d "$DIR" ]] || fail "Backup directory not found: $DIR"
[[ -f "$DIR/flash.bin" ]] || fail "flash.bin not found in: $DIR"

PORT="$(require_port)"

if [[ -f "$DIR/sha256.txt" ]]; then
  info "Verifying backup SHA-256..."
  (cd "$DIR" && shasum -a 256 -c sha256.txt)
fi

printf '\nThis will replace the ENTIRE Heltec flash with:\n  %s\nDevice:\n  %s\n\n'   "$DIR/flash.bin" "$PORT"

read -r -p "Type RESTORE to continue: " answer
[[ "$answer" == "RESTORE" ]] || fail "Restore cancelled"

run_esptool_action "$PORT" write-flash write_flash 0 "$DIR/flash.bin"
ok "Restore complete"
