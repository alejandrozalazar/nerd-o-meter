#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

INSTALL=0
REQUIRE_DEVICE=0
SKIP_BUILD=0

for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    --device) REQUIRE_DEVICE=1 ;;
    --no-build) SKIP_BUILD=1 ;;
    *) fail "Unknown option: $arg" ;;
  esac
done

cd "$ROOT"
info "Nerd-O-Meter preflight"
printf 'Repository: %s\n\n' "$ROOT"

[[ "$(uname -s)" == "Darwin" ]] ||
  warn "Optimized for macOS; detected $(uname -s)."

if [[ "$(uname -m)" == "arm64" ]]; then
  ok "Apple Silicon detected ($(uname -m))"
else
  warn "Expected Apple Silicon arm64; detected $(uname -m)."
fi

[[ -w "$ROOT" ]] || fail "Repository is not writable: $ROOT"
ok "Repository is writable"

free_kb="$(df -Pk "$ROOT" | awk 'NR==2 {print $4}')"
free_mb="$((free_kb / 1024))"
if (( free_mb < 1024 )); then
  warn "Only ${free_mb} MB free. PlatformIO toolchains may need more space."
else
  ok "Free disk space: ${free_mb} MB"
fi

if command -v git >/dev/null 2>&1; then
  ok "git: $(git --version)"
else
  fail "git is missing. Run: xcode-select --install"
fi

if command -v brew >/dev/null 2>&1; then
  ok "Homebrew: $(brew --version | head -n 1)"
else
  fail "Homebrew is required for the fast setup. Install it from https://brew.sh/ and rerun."
fi

install_formula() {
  local formula="$1"
  local command_name="$2"

  if command -v "$command_name" >/dev/null 2>&1; then
    ok "$command_name: $("$command_name" --version 2>&1 | head -n 1)"
    return
  fi

  if [[ $INSTALL -eq 1 ]]; then
    info "Installing $formula with Homebrew..."
    brew install "$formula"
    ok "$formula installed"
  else
    warn "$command_name missing. Install with: brew install $formula"
    return 1
  fi
}

missing=0
install_formula platformio pio || missing=1

if [[ -n "$(find_esptool)" ]]; then
  ok "esptool: $($(find_esptool) version 2>&1 | head -n 1 || true)"
else
  if [[ $INSTALL -eq 1 ]]; then
    info "Installing esptool with Homebrew..."
    brew install esptool
    ok "esptool installed"
  else
    warn "esptool missing. Install with: brew install esptool"
    missing=1
  fi
fi

if ! command -v python3 >/dev/null 2>&1; then
  warn "python3 missing. Homebrew PlatformIO normally installs Python automatically."
  missing=1
else
  ok "python3: $(python3 --version)"
fi

if (( missing != 0 )); then
  printf '\nFast path:\n  bash scripts/preflight.sh --install\n'
  exit 2
fi

[[ -f platformio.ini ]] || fail "platformio.ini missing"
ok "platformio.ini present"

info "Resolving PlatformIO platform + libraries..."
pio pkg install
ok "PlatformIO dependencies resolved"

if [[ $SKIP_BUILD -eq 0 ]]; then
  info "Compiling once now so flashing later is boring..."
  pio run
  ok "Firmware build passed"
fi

port="$(find_port || true)"
if [[ -n "$port" ]]; then
  ok "Serial device: $port"
  info "Probing ESP32-S3 (read-only)..."

  if run_esptool_action "$port" flash-id flash_id >/tmp/nerdometer-esptool.txt 2>&1; then
    ok "ESP32-S3 responded to esptool"
    sed -n '1,14p' /tmp/nerdometer-esptool.txt | sed 's/^/       /'
  else
    warn "Port exists but esptool probe failed. If needed: hold PRG, tap RST, release PRG, retry."
  fi
elif [[ $REQUIRE_DEVICE -eq 1 ]]; then
  fail "No Heltec serial device detected. Connect the board and rerun."
else
  warn "No Heltec connected (fine for a build-only preflight)."
fi

printf '\n'
ok "Preflight complete"
printf 'Next with the Heltec connected:\n'
printf '  bash scripts/backup.sh\n'
printf '  bash scripts/flash.sh\n'
