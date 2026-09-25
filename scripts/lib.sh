#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

info() { printf '\033[1;36m[INFO]\033[0m %s\n' "$*"; }
ok()   { printf '\033[1;32m[ OK ]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[WARN]\033[0m %s\n' "$*" >&2; }
fail() { printf '\033[1;31m[FAIL]\033[0m %s\n' "$*" >&2; exit 1; }

find_esptool() {
  command -v esptool 2>/dev/null || command -v esptool.py 2>/dev/null || true
}

run_esptool_action() {
  local port="$1"
  local modern="$2"
  local legacy="$3"
  shift 3

  local bin action
  bin="$(find_esptool)"
  [[ -n "$bin" ]] || fail "esptool not found. Install with: brew install esptool"

  action="$legacy"
  if "$bin" --help 2>&1 | grep -q -- "$modern"; then
    action="$modern"
  fi

  "$bin" --chip esp32s3 --port "$port" "$action" "$@"
}

find_port() {
  if [[ -n "${NERDOMETER_PORT:-}" ]]; then
    printf '%s\n' "$NERDOMETER_PORT"
    return 0
  fi

  if command -v pio >/dev/null 2>&1 && command -v python3 >/dev/null 2>&1; then
    local json port
    json="$(pio device list --json-output 2>/dev/null || true)"

    if [[ -n "$json" ]]; then
      port="$(python3 -c '
import json, sys
try:
    devs = json.load(sys.stdin)
except Exception:
    raise SystemExit(1)

preferred = []
fallback = []
for d in devs:
    p = d.get("port", "")
    text = (d.get("description", "") + " " + d.get("hwid", "")).lower()
    if not p.startswith("/dev/cu."):
        continue
    if "cp210" in text or "10c4" in text or "silicon labs" in text:
        preferred.append(p)
    elif "usb" in p.lower() or "usb" in text or "esp32" in text:
        fallback.append(p)

choices = preferred or fallback
if choices:
    print(choices[0])
' <<<"$json" 2>/dev/null || true)"

      if [[ -n "$port" ]]; then
        printf '%s\n' "$port"
        return 0
      fi
    fi
  fi

  local candidates=()
  while IFS= read -r p; do
    candidates+=("$p")
  done < <(
    compgen -G '/dev/cu.SLAB_USBtoUART*' || true
    compgen -G '/dev/cu.usbserial*' || true
    compgen -G '/dev/cu.usbmodem*' || true
  )

  if [[ ${#candidates[@]} -ge 1 ]]; then
    printf '%s\n' "${candidates[0]}"
    return 0
  fi

  return 1
}

require_port() {
  local port
  port="$(find_port || true)"
  [[ -n "$port" ]] ||
    fail "Heltec serial port not detected. Connect it by USB-C, or set NERDOMETER_PORT=/dev/cu...."
  printf '%s\n' "$port"
}
