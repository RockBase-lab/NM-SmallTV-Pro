import os
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('src/main.cpp').read_text(encoding='utf-8')


def function(name):
    match = re.search(r'^(?:static )?[^\n]+\b' + name + r'\([^\n]*\) \{[\s\S]*?^\}', source, re.M)
    assert match, name
    return match.group()


fixture = r'''
#include "Settings.h"
#include <cassert>
#include <ctime>
#include <cstdio>
static uint32_t nowMs = 100;
uint32_t millis() { return nowMs; }
static bool synced = true;
static int minute = 0;
bool clockNow(tm& now) { now.tm_hour = minute / 60; now.tm_min = minute % 60; return synced; }
struct DisplayMode {
  unsigned mode, wakes = 0;
  const char* name;
  uint8_t modeConst() const { return mode; }
  const char* id() const { return name; }
  void wake(const Settings&) { ++wakes; }
  void invalidate(const Settings&) {}
};
static DisplayMode stocks{MODE_STOCKS,0,"stocks"}, stocks2{MODE_STOCKS2,0,"stocks2"},
 crypto{MODE_CRYPTO,0,"crypto"}, weather{MODE_WEATHER,0,"weather"},
 usage{MODE_USAGE,0,"usage"}, radar{MODE_RADAR,0,"radar"}, ha{MODE_HA,0,"ha"},
 theme{MODE_THEME,0,"theme"}, pictures{MODE_PICTURES,0,"pictures"};
static DisplayMode* kModes[] = {&stocks,&stocks2,&crypto,&weather,&usage,&radar,&ha,&theme,&pictures};
static const size_t kModeCount = 9;
static size_t g_carIdx = 0;
static uint32_t g_carSwitch = 0, g_manualUntil = 0;
static bool g_carPaused = false;
static int g_manualIdx = -1;
static uint8_t g_lastTimerMode = 255;
static DisplayMode* g_visibleMode = nullptr;
static bool g_modeMessageDrawn = false;
static Settings g_settings;
static bool g_abortFetch = false;
static const unsigned NUM_STOCK_SETS = 3;
void stocksDeferNextFetch(unsigned, unsigned) {}
void weatherDeferNextFetch(unsigned) {}
void buttonBlinkOff() {}
void buttonShowNetInfo() {}
void buttonWake() {}
void buttonAckNotify() {}
'''

checks = r'''
int main() {
  Settings& settings = g_settings;
  settings.mode = MODE_CAROUSEL;
  settings.carouselTicker = settings.carouselTicker2 = settings.carouselCrypto = false;
  settings.carouselUsage = settings.carouselRadar = settings.carouselHa = settings.carouselPictures = false;
  settings.carouselWeather = settings.carouselTheme = true;
  settings.carouselSec = 30;
  assert(activeMode(settings) == &weather);
  buttonNext();
  assert(activeMode(settings) == &theme);
  nowMs += 30000;
  assert(activeMode(settings) == &weather);
  settings.carouselWeather = settings.carouselTheme = false;
  assert(activeMode(settings) == nullptr);
  buttonNext();
  assert(activeMode(settings) == nullptr);
  settings.carouselPictures = true;
  buttonGoHome();
  assert(activeMode(settings) == &pictures);
  nowMs += 60000;
  assert(activeMode(settings) == &pictures);
  settings.carouselWeather = true;
  nowMs += 1000;
  appInvalidate();
  assert(g_carSwitch == nowMs);
  assert(activeMode(settings) == &weather);
  nowMs += 29999;
  assert(activeMode(settings) == &weather);
  nowMs += 1;
  assert(activeMode(settings) == &pictures);

  settings.mode = MODE_TIMER;
  g_manualIdx = -1;
  for (unsigned index = 0; index < 4; ++index) settings.timers.items[index].enabled = false;
  assert(activeMode(settings) == nullptr);
  settings.timers.items[0] = {true,"06:00","radar"};
  settings.timers.items[1] = {true,"23:00","weather"};
  minute = 5 * 60;
  assert(activeMode(settings) == &weather);
  minute = 6 * 60;
  assert(activeMode(settings) == &radar);
  buttonNext();
  assert(activeMode(settings) == &ha);
  nowMs += 15000;
  assert(activeMode(settings) == &radar);
  synced = false;
  assert(activeMode(settings) == nullptr);
  synced = true;
  settings.timers.items[0].time = "99:99";
  assert(activeMode(settings) == &weather);
  assert(modeFromApp("usage") == MODE_USAGE);
  assert(modeFromApp("ha") == MODE_HA);
  assert(modeFromApp("invalid") == 255);

  settings.mode = MODE_STOCKS;
  buttonAction(BTN_ACT_NONE);
  assert(!g_abortFetch);
  buttonAction(BTN_ACT_HOME);
  assert(!g_abortFetch);
  assert(activeMode(settings) == &stocks);
  buttonNext();
  assert(activeMode(settings) == &stocks2);
  buttonGoHome();
  assert(activeMode(settings) == &stocks);
  puts("Carousel selection, touch, timer, save dwell and fetch cancellation passed");
}
'''

functions = '\n'.join(function(name) for name in (
    'carouselHas', 'carouselNext', 'modeFromApp', 'resolveTimerMode',
    'activeMode', 'buttonGoHome', 'buttonNext', 'buttonAction', 'appInvalidate'))
timeout = re.search(r'  if \(g_btnInfoUntil && millis\(\) >= g_btnInfoUntil\) \{[\s\S]*?\n  \}', source).group()
fixture += r'''
static unsigned g_btnInfoUntil = 0;
static int networkMode = 1, apPaints = 0;
enum { NET_AP = 0 };
int netMode() { return networkMode; }
String netIP() { return "192.168.4.1"; }
const char* netFailReason() { return ""; }
void gfxApInfo(const char*, const char*, const char*, const char*) { ++apPaints; }
'''
functions += '\nvoid expireNetworkInfo() {\n' + timeout + '\n}\n'
checks = checks.replace('  puts(', r'''
  g_visibleMode = nullptr;
  g_modeMessageDrawn = true;
  g_btnInfoUntil = nowMs;
  expireNetworkInfo();
  assert(g_btnInfoUntil == 0 && !g_modeMessageDrawn);
  g_visibleMode = &weather;
  g_btnInfoUntil = nowMs;
  expireNetworkInfo();
  assert(g_visibleMode == nullptr);
  networkMode = NET_AP;
  g_btnInfoUntil = nowMs;
  expireNetworkInfo();
  assert(apPaints == 1);
  puts(''')
with tempfile.TemporaryDirectory() as directory:
    test_source = Path(directory) / 'dispatch.cpp'
    executable = Path(directory) / 'dispatch'
    test_source.write_text(fixture + functions + checks, encoding='utf-8')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++14', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-variable', '-Wno-unused-parameter', '-DSMALLTV_ESP8266',
                    '-DWITH_HA=1', '-DWITH_CRYPTO=1', '-DWITH_PICTURES=1',
                    '-Itests/fuzz/shim', '-Isrc', '-I.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src',
                    str(test_source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
