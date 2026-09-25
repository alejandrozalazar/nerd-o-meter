#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

PORT="$(require_port)"

printf 'Configure Wi-Fi/NTP for Nerd-O-Meter\n'
printf 'Device: %s\n\n' "$PORT"

IFS= read -r -p "Wi-Fi SSID: " SSID
IFS= read -r -s -p "Wi-Fi password: " PASSWORD
printf '\n'

[[ -n "$SSID" ]] || fail "SSID cannot be empty"
[[ "$SSID" != *$'\t'* && "$SSID" != *$'\n'* ]] ||
  fail "SSID cannot contain tabs/newlines"
[[ "$PASSWORD" != *$'\t'* && "$PASSWORD" != *$'\n'* ]] ||
  fail "Password cannot contain tabs/newlines"

export NERDOMETER_WIFI_SSID="$SSID"
export NERDOMETER_WIFI_PASSWORD="$PASSWORD"

python3 - "$PORT" <<'PY'
import os
import sys
import termios
import time

port = sys.argv[1]
ssid = os.environ["NERDOMETER_WIFI_SSID"]
password = os.environ["NERDOMETER_WIFI_PASSWORD"]
payload = f"WIFI\t{ssid}\t{password}\n".encode("utf-8")

fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
try:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
    attrs[3] = 0
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    time.sleep(0.15)
    os.write(fd, payload)
    termios.tcdrain(fd)
    time.sleep(0.2)
finally:
    os.close(fd)
PY

unset NERDOMETER_WIFI_SSID NERDOMETER_WIFI_PASSWORD PASSWORD

ok "Credentials sent to the Heltec"
printf '%s\n'   "The firmware stores them only in ESP32 NVS; this script does not write them to the repository."   "The board immediately attempts NTP. To inspect the result:"   "  bash scripts/monitor.sh"
