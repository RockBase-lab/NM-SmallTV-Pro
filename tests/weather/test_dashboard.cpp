#include "WeatherClock.h"
#include <cassert>
#include <cstring>
#include <set>
#include <vector>

class SpriteCanvas : public WeatherCanvas {
 public:
  std::vector<uint16_t> pixels = std::vector<uint16_t>(240 * 240);
  unsigned rows = 0;
  void rectangle(int, int, int, int, uint16_t, bool) override {}
  void circle(int, int, int, uint16_t, bool) override {}
  void line(int, int, int, int, uint16_t) override {}
  void triangle(int, int, int, int, int, int, uint16_t) override {}
  void text(int, int, const char*, unsigned, uint16_t) override {}
  void rasterRow(int x, int y, const uint16_t* colors, int count) override {
    assert(x >= 180 && x + count <= 236 && y >= 180 && y < 236);
    for (int column = 0; column < count; ++column) pixels[y * 240 + x + column] = colors[column];
    ++rows;
  }
};

int main() {
  WeatherClockView view;
  view.weatherValid = true;
  std::strcpy(view.weather.sunrise, "07:04");
  view.ipAddress = "192.168.124.19";
  char info[40];
  for (uint32_t elapsed : {0u, 9999u, 20000u}) {
    view.elapsedMs = elapsed;
    formatWeatherDashboardInfo(view, info, sizeof(info));
    assert(std::strcmp(info, "Sunrise 07:04") == 0);
  }
  for (uint32_t elapsed : {10000u, 19999u, 30000u}) {
    view.elapsedMs = elapsed;
    formatWeatherDashboardInfo(view, info, sizeof(info));
    assert(std::strcmp(info, "IP 192.168.124.19") == 0);
  }
  view.ipAddress = "255.255.255.255";
  formatWeatherDashboardInfo(view, info, sizeof(info));
  assert(std::strcmp(info, "IP 255.255.255.255") == 0);
  view.ipAddress = "";
  formatWeatherDashboardInfo(view, info, sizeof(info));
  assert(std::strcmp(info, "IP offline") == 0);
  view.weatherValid = false;view.elapsedMs = 0;
  formatWeatherDashboardInfo(view, info, sizeof(info));
  assert(std::strcmp(info, "Sunrise --:--") == 0);
  assert(weatherSpacemanFrame(0) == 0);
  assert(weatherSpacemanFrame(79) == 0);
  assert(weatherSpacemanFrame(80) == 1);
  assert(weatherSpacemanFrame(1119) == 13);
  assert(weatherSpacemanFrame(1120) == 0);
  std::set<std::vector<uint16_t>> frames;
  for (uint32_t elapsed = 0; elapsed < 1120; elapsed += 80) {
    SpriteCanvas canvas;
    renderWeatherSpaceman(canvas, weatherSpacemanFrame(elapsed));
    assert(canvas.rows == 56);
    frames.insert(canvas.pixels);
  }
  assert(frames.size() == 14);
}
