import os
from pathlib import Path
import re
import subprocess
import tempfile


source = Path("src/Clock.cpp").read_text(encoding="utf-8")
reapply = re.search(r"void clockReapply\([^\n]+\) \{[\s\S]*?\n\}", source).group()
fixture = r'''
#include <cassert>
#include <ctime>
#include <string>
#define WITH_THEME 1
enum { MODE_STOCKS, MODE_THEME, MODE_CAROUSEL, MODE_TIMER, MODE_WEATHER };
struct Settings {
  int mode = MODE_STOCKS;
  bool carouselTheme = false, carouselWeather = false;
  struct { bool nightEnabled = false, sunEnabled = false; std::string tzPosix, ntpServer; } clock;
  struct { bool enabled = false; std::string triggerType; } burnin;
};
bool wgNeedsClock(const Settings&) { return false; }
bool s_ntpStarted = false;
time_t s_sunAt = 0;
std::string s_armedTz, s_armedNtp;
unsigned starts = 0;
void clockBegin(const Settings& settings) {
  ++starts;
  s_ntpStarted = true;
  s_armedTz = settings.clock.tzPosix;
  s_armedNtp = settings.clock.ntpServer;
}
'''
checks = r'''
int main() {
  Settings settings;
  clockReapply(settings, false);
  assert(starts == 0);
  clockReapply(settings, true);
  assert(starts == 1);
  clockReapply(settings, true);
  assert(starts == 1);
  settings.clock.tzPosix = "CST-8";
  clockReapply(settings, true);
  assert(starts == 2);
  settings.clock.ntpServer = "ntp.example.com";
  clockReapply(settings, true);
  assert(starts == 3);
  s_ntpStarted = false;
  clockReapply(settings, false);
  assert(starts == 3);
  settings.mode = MODE_WEATHER;
  clockReapply(settings, false);
  assert(starts == 4);
  s_ntpStarted = false;
  settings.mode = MODE_CAROUSEL;
  settings.carouselWeather = true;
  clockReapply(settings, false);
  assert(starts == 5);
}
'''
with tempfile.TemporaryDirectory() as directory:
    test_source = Path(directory) / "clock.cpp"
    executable = Path(directory) / "clock-test"
    test_source.write_text(fixture + reapply + checks)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", str(test_source), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)

wake = Path("src/features/weather/WeatherMode.cpp").read_text(encoding="utf-8").split("void WeatherMode::wake(")[1].split("void WeatherMode::render(")[0]
assert "clockReapply(settings, true)" in wake
theme = Path("src/features/theme/ThemeMode.cpp").read_text(encoding="utf-8")
theme_wake = re.search(r"void ThemeMode::wake\([^\n]+\) \{[\s\S]*?\n\}", theme).group()
theme_fixture = r'''
#include <cassert>
struct Settings {};
unsigned starts = 0;
void clockReapply(const Settings&, bool force) { assert(force); ++starts; }
struct ThemeMode {
  bool pictures_, messageDrawn_ = true;
  struct Engine { unsigned paints = 0; void invalidate() { ++paints; } } engine_;
  void wake(const Settings&);
};
int main() {
  Settings settings;
  ThemeMode theme{}, picture{};
  theme.pictures_ = false;
  picture.pictures_ = true;
  theme.wake(settings);
  assert(starts == 1 && theme.engine_.paints == 1 && !theme.messageDrawn_);
  picture.wake(settings);
  assert(starts == 1 && picture.engine_.paints == 1 && !picture.messageDrawn_);
}
'''
with tempfile.TemporaryDirectory() as directory:
    test_source = Path(directory)/"theme-wake.cpp"
    executable = Path(directory)/"theme-wake"
    test_source.write_text(theme_fixture + theme_wake)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++14", "-Wall", "-Wextra", "-Werror",
                    str(test_source), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("Weather clock demand tests passed")
