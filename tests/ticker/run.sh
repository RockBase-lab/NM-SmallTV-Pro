#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
JSON_INCLUDE=${THEME_JSON_INCLUDE:-.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src}
c++ -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter \
  -DSMALLTV_ESP8266 -DARDUINOJSON_USE_DOUBLE=0 -DARDUINOJSON_ENABLE_ARDUINO_STRING=1 \
  -Itests/fuzz/shim -Isrc -Isrc/features/theme -I"$JSON_INCLUDE" \
  tests/ticker/test_defaults.cpp src/Settings.cpp -lm -o /tmp/smalltv-stock-defaults
/tmp/smalltv-stock-defaults
python3 tests/ticker/test_providers.py
