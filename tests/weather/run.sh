#!/bin/sh
# Weather display conversion and the browser-side settings round trip.
set -eu
cd "$(dirname "$0")/../.."
c++ -std=c++11 -Wall -Wextra -Werror -g -Isrc tests/weather/test_units.cpp -o /tmp/smalltv-weather-unit-tests
/tmp/smalltv-weather-unit-tests
JSON_INCLUDE=${THEME_JSON_INCLUDE:-.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src}
c++ -std=c++11 -Wall -Wextra -Werror -Isrc/features/weather -I"$JSON_INCLUDE" tests/weather/test_open_meteo.cpp -o /tmp/smalltv-weather-parser-tests
/tmp/smalltv-weather-parser-tests
c++ -std=c++11 -Wall -Wextra -Werror -Isrc/features/weather -I"$JSON_INCLUDE" tests/weather/test_render.cpp src/features/weather/WeatherClock.cpp -o /tmp/smalltv-weather-render-tests
/tmp/smalltv-weather-render-tests
c++ -std=c++11 -Wall -Wextra -Werror -Isrc/features/weather -I"$JSON_INCLUDE" tests/weather/test_dashboard.cpp src/features/weather/WeatherClock.cpp -o /tmp/smalltv-weather-dashboard-tests
/tmp/smalltv-weather-dashboard-tests
python3 tests/weather/test_startup.py
python3 tests/weather/test_location.py
python3 tests/weather/test_spaceman_asset.py
node tests/weather/test_webui.js
node tests/weather/test_clocks.js
python3 tests/weather/test_clock_demand.py
