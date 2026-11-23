#include "WeatherClock.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

class RecordingCanvas : public WeatherCanvas {
 public:
  bool recording = false;
  std::string labels;
  unsigned operations = 0;
  void rasterRow(int x, int y, const uint16_t* pixels, int count) override {
    point(x, y);point(x+count-1,y);++operations;
    if (recording) {
      std::printf("[\"row\",%d,%d,[",x,y);
      for (int column=0;column<count;++column) std::printf("%s%u",column?",":"",pixels[column]);
      std::printf("]]\n");
    }
  }
  void point(int x, int y) { assert(x >= 0 && x < 240 && y >= 0 && y < 240); }
  void rectangle(int x, int y, int width, int height, uint16_t color, bool fill) override {
    assert(width >= 0 && height >= 0);
    point(x, y);
    if (width && height) point(x + width - 1, y + height - 1);
    ++operations;
    if (recording) std::printf("[\"rect\",%d,%d,%d,%d,%u,%d]\n", x, y, width, height, color, fill);
  }
  void circle(int x, int y, int radius, uint16_t color, bool fill) override {
    point(x - radius, y - radius); point(x + radius, y + radius); ++operations;
    if (recording) std::printf("[\"circle\",%d,%d,%d,%u,%d]\n", x, y, radius, color, fill);
  }
  void line(int x, int y, int endX, int endY, uint16_t color) override {
    point(x, y); point(endX, endY); ++operations;
    if (recording) std::printf("[\"line\",%d,%d,%d,%d,%u]\n", x, y, endX, endY, color);
  }
  void triangle(int x, int y, int secondX, int secondY, int thirdX, int thirdY, uint16_t color) override {
    point(x, y); point(secondX, secondY); point(thirdX, thirdY); ++operations;
    if (recording) std::printf("[\"triangle\",%d,%d,%d,%d,%d,%d,%u]\n", x, y, secondX, secondY, thirdX, thirdY, color);
  }
  void text(int x, int y, const char* value, unsigned size, uint16_t color) override {
    point(x, y);
    if (*value) point(x + std::strlen(value) * 6 * size - 1, y + 8 * size - 1);
    labels += value; labels += '\n'; ++operations;
    if (recording) std::printf("[\"text\",%d,%d,%u,%u,\"%s\"]\n", x, y, size, color, value);
  }
};

int main(int argc, char** argv) {
  WeatherClockView view;
  view.clockValid = view.weatherValid = true;
  view.time.tm_hour = 12; view.time.tm_min = 48; view.time.tm_sec = 36;
  view.time.tm_year = 126; view.time.tm_mon = 10; view.time.tm_mday = 20; view.time.tm_wday = 5;
  view.location = "Beijing";
  view.weather.temp = 22; view.weather.humidity = 66; view.weather.windSpeed = 12;
  view.weather.weatherCode = 2; view.weather.forecastCount = 3;
  std::strcpy(view.weather.sunrise, "07:04");
  view.ipAddress = "192.168.124.19";
  if (argc > 1 && std::strcmp(argv[1], "dashboard-animation") == 0) {
    view.style = WEATHER_DASHBOARD;
    for (unsigned frame = 0; frame < 28; ++frame) {
      view.elapsedMs = frame % 14 * 80 + (frame >= 14 ? 10000 : 0);
      RecordingCanvas canvas;
      canvas.recording = true;
      std::printf("[\"style\",%u]\n", frame);
      renderWeatherClock(canvas, view);
    }
    return 0;
  }
  for (unsigned day = 0; day < 3; ++day) {
    std::snprintf(view.weather.forecast[day].date, 11, "2026-11-%02u", 20 + day);
    view.weather.forecast[day].tempMin = 8 + day;
    view.weather.forecast[day].tempMax = 22 - day;
    view.weather.forecast[day].weatherCode = day == 0 ? 0 : day == 1 ? 3 : 61;
  }
  for (unsigned style = 0; style < 4; ++style) {
    view.style = style;
    RecordingCanvas canvas;
    canvas.recording = argc > 1;
    if (canvas.recording) std::printf("[\"style\",%u]\n", style);
    renderWeatherClock(canvas, view);
    assert(canvas.operations > 20);
  }
  RecordingCanvas canvas;
  const unsigned codes[] = {0,1,2,3,45,48,51,56,61,67,71,77,80,85,95,99};
  view.location = "An extremely long location name that must be cropped safely";
  for (unsigned style = 0; style < 4; ++style) {
    view.style = style;
    for (unsigned unit = 0; unit < 2; ++unit) {
      view.tempUnit = unit;
      for (unsigned code : codes) {
        view.weather.weatherCode = code;
        view.weather.isDay = false;
        view.weather.temp = -100;
        view.weather.forecast[0].tempMin = -100;
        view.weather.forecast[0].tempMax = 100;
        for (int hour : {0,12,23}) {
          view.time.tm_hour = hour;
          view.time.tm_min = hour == 23 ? 59 : 0;
          renderWeatherClock(canvas, view);
        }
      }
    }
    view.weatherValid = false;
    view.clockValid = false;
    view.error = true;
    canvas.labels.clear();
    renderWeatherClock(canvas, view);
    assert(canvas.labels.find("0C") == std::string::npos);
    if (style != WEATHER_STACKED) assert(canvas.labels.find("Weather unavailable") != std::string::npos);
    view.weatherValid = view.clockValid = true;
  }
  canvas.operations = 0;
  renderWeatherSeconds(canvas, 59, true);
  assert(canvas.operations < 90);
  std::fprintf(stderr, "Weather renderer bounds and state tests passed\n");
}
