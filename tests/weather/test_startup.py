from pathlib import Path
import os
import subprocess
import tempfile

fixture = '''
#include <cassert>
#include <cstring>
#include "CONFIG"
#include "STYLE"
#ifndef DEFAULT_LOCATION_LAT
#define DEFAULT_LOCATION_LAT 0.0f
#define DEFAULT_LOCATION_LON 0.0f
#define DEFAULT_LOCATION_NAME ""
#endif
int main() {
#if defined(SMALLTV_ESP32) && !defined(SMALLTV_ESP32_PRO)
  assert(DEFAULT_MODE == MODE_WEATHER && DEFAULT_WEATHER_STYLE == WEATHER_DASHBOARD);
  assert(!std::strcmp(DEFAULT_TZ_NAME, "Asia/Shanghai") && !std::strcmp(DEFAULT_TZ_POSIX, "CST-8"));
  assert(DEFAULT_LOCATION_LAT > 30.57f && DEFAULT_LOCATION_LAT < 30.58f);
  assert(DEFAULT_LOCATION_LON > 104.06f && DEFAULT_LOCATION_LON < 104.07f);
  assert(!std::strcmp(DEFAULT_LOCATION_NAME, "Chengdu, China"));
#else
  assert(DEFAULT_MODE == MODE_TIMER && DEFAULT_WEATHER_STYLE == WEATHER_SIMPLE);
#endif
}
'''.replace('"CONFIG"', '"' + Path('src/config.h').resolve().as_posix() + '"').replace('"STYLE"', '"' + Path('src/features/weather/WeatherClock.h').resolve().as_posix() + '"')

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory)/'startup.cpp'
    source.write_text(fixture)
    for definitions in (['SMALLTV_ESP32'], ['SMALLTV_ESP32','SMALLTV_ESP32_WG'],
                        ['SMALLTV_ESP32','SMALLTV_ESP32_PRO'], ['SMALLTV_ESP8266'], ['SMALLTV_ESP32C2']):
        output = Path(directory)/'startup'
        subprocess.run([os.environ.get('CXX','c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                        *['-D'+definition for definition in definitions], str(source), '-o', str(output)], check=True)
        subprocess.run([str(output)], check=True)

main = Path('src/main.cpp').read_text(encoding='utf-8')
assert main.index('loadSettings(g_settings);') < main.index('gfxBegin(g_settings);')
assert 'weatherStartup(' not in main
print('Weather startup policy tests passed')
