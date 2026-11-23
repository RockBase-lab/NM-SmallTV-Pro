#pragma once
#include "WeatherForecast.h"
#include <ctime>
#include <cstddef>

class WeatherCanvas {
 public:
  virtual ~WeatherCanvas() {}
  virtual void rectangle(int x, int y, int width, int height, uint16_t color, bool fill) = 0;
  virtual void circle(int x, int y, int radius, uint16_t color, bool fill) = 0;
  virtual void line(int x, int y, int endX, int endY, uint16_t color) = 0;
  virtual void triangle(int x, int y, int secondX, int secondY, int thirdX, int thirdY, uint16_t color) = 0;
  virtual void text(int x, int y, const char* value, unsigned size, uint16_t color) = 0;
  virtual void rasterRow(int x, int y, const uint16_t* pixels, int count) {
    for (int column = 0; column < count; ++column) rectangle(x + column, y, 1, 1, pixels[column], true);
  }
};

struct WeatherClockView {
  WeatherObservation weather;
  tm time{};
  const char* location = "Weather";
  const char* ipAddress = "";
  uint32_t elapsedMs = 0;
  bool clockValid = false;
  bool weatherValid = false;
  bool error = false;
  uint8_t style = WEATHER_SIMPLE;
  uint8_t tempUnit = 0;
  uint8_t windUnit = 0;
};

void renderWeatherClock(WeatherCanvas& canvas, const WeatherClockView& view);
void renderWeatherSeconds(WeatherCanvas& canvas, int seconds, bool valid);
void formatWeatherDashboardInfo(const WeatherClockView& view, char* output, size_t size);
void renderWeatherDashboardInfo(WeatherCanvas& canvas, const WeatherClockView& view);
uint8_t weatherSpacemanFrame(uint32_t elapsedMs);
void renderWeatherSpaceman(WeatherCanvas& canvas, uint8_t frame);
