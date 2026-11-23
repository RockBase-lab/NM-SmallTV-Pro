import os
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('src/features/weather/WeatherClient.cpp').read_text(encoding='utf-8')
service = re.search(r'void weatherService\([^\n]+\) \{[\s\S]*?\n\}', source).group()
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cmath>
struct Settings { struct { float lat = 0, lon = 0; } clock; unsigned weatherPollMin = 10, httpTimeout = 8000; };
struct WeatherData { uint32_t nextTryMs = 0, lastOkMs = 0; unsigned fails = 0; bool valid = false, error = false; } g_weather;
bool g_fetching = false, g_abortFetch = false;
unsigned calls = 0;
float fetchedLat = 0, fetchedLon = 0;
uint32_t millis() { return 1000; }
bool fetchOpenMeteo(float lat, float lon, unsigned, WeatherData&) { ++calls; fetchedLat = lat; fetchedLon = lon; return true; }
'''
header = Path('src/features/weather/WeatherLocation.h')
if header.exists():
    fixture += '#include "' + header.resolve().as_posix() + '"\n'
checks = r'''
int main() {
  Settings settings;
  weatherService(settings);
  assert(calls == 0 && !g_weather.valid);
  settings.clock.lon = 15;
  weatherService(settings);
  assert(calls == 1 && fetchedLat == 0 && fetchedLon == 15);
  settings.clock.lat = 52;
  settings.clock.lon = 0;
  g_weather.nextTryMs = 0;
  weatherService(settings);
  assert(calls == 2 && fetchedLat == 52 && fetchedLon == 0);
  settings.clock.lat = 91;
  g_weather.nextTryMs = 0;
  weatherService(settings);
  assert(calls == 2);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)/'location.cpp'
    binary = Path(directory)/'location'
    path.write_text(fixture + service + checks)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Weather service rejects unset locations and preserves equator/Greenwich coordinates')
