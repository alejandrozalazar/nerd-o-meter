#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

bash scripts/preflight.sh --device
bash scripts/backup.sh
bash scripts/flash.sh
