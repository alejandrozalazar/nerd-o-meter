#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

PORT="$(require_port)"

python3 - "$PORT" <<'PY'
import os
import sys
import termios
import time

port = sys.argv[1]
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
    os.write(fd, b"CLEAR_WIFI\n")
    termios.tcdrain(fd)
    time.sleep(0.2)
finally:
    os.close(fd)
PY

ok "Requested Wi-Fi credential erase from ESP32 NVS"
