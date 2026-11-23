#!/bin/sh
set -eu
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
c++ -std=c++14 -Wall -Wextra -Werror -DARDUINOJSON_ENABLE_ARDUINO_STRING=1 -Itests/fuzz/shim -Isrc -I.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src tests/search/test_stock_search.cpp -o "$bin"
"$bin"
echo 'Stock search query bounds, URL encoding, stream limit and provider normalization passed'
