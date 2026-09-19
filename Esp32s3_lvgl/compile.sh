#!/usr/bin/env bash
# Usage: ./compile.sh [conf|compile|upload|monitor|all]
# Run from inside the esp32_lvgl folder. Confirm option names with:
#   arduino-cli board details -b esp32:esp32:esp32s3
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc"
PORT="${PORT:-/dev/ttyACM0}"
SKETCH="$(cd "$(dirname "$0")" && pwd)"

case "${1:-all}" in
  conf)    # install the ESP32 lv_conf.h next to the lvgl library (backs up an existing one)
           LIBS="${HOME}/Arduino/libraries"
           [ -f "$LIBS/lv_conf.h" ] && cp "$LIBS/lv_conf.h" "$LIBS/lv_conf.h.bak"
           cp "$SKETCH/../lvgl_config/lv_conf.h" "$LIBS/lv_conf.h" && echo "installed $LIBS/lv_conf.h" ;;
  compile) arduino-cli compile --fqbn "$FQBN" --output-dir "$SKETCH/build" "$SKETCH" ;;
  upload)  arduino-cli upload  -p "$PORT" --fqbn "$FQBN" --input-dir "$SKETCH/build" "$SKETCH" ;;
  monitor) arduino-cli monitor -p "$PORT" -c baudrate=115200 ;;
  all)     "$0" compile && "$0" upload && "$0" monitor ;;
  *)       echo "usage: $0 [conf|compile|upload|monitor|all]"; exit 1 ;;
esac
