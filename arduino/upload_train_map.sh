#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ARDUINO_CLI="/home/cleger/Dev/arduino/bin/arduino-cli"
SKETCH_DIR="$SCRIPT_DIR/train_map"
FQBN="arduino:avr:uno"
PORT="${1:-/dev/ttyUSB0}"

"$ARDUINO_CLI" compile --fqbn "$FQBN" "$SKETCH_DIR"
"$ARDUINO_CLI" upload --fqbn "$FQBN" --port "$PORT" "$SKETCH_DIR"
