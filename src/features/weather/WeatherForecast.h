#pragma once
#include <stdint.h>

struct WeatherDay {
  char date[11] = {};
  float tempMin = 0;
  float tempMax = 0;
  uint8_t weatherCode = 0;
};

struct WeatherObservation {
  float temp = 0;
  uint8_t humidity = 0;
  uint8_t weatherCode = 0;
  float windSpeed = 0;
  float tempMax = 0;
  float tempMin = 0;
  bool isDay = true;
  char sunrise[6] = "--:--";
  char sunset[6] = "--:--";
  uint8_t forecastCount = 0;
  WeatherDay forecast[3];
};

enum WeatherStyle : uint8_t {
  WEATHER_SIMPLE = 0,
  WEATHER_DASHBOARD = 1,
  WEATHER_FORECAST = 2,
  WEATHER_STACKED = 3
};

inline const char* weatherStyleName(uint8_t style) {
  switch (style) {
    case WEATHER_DASHBOARD: return "dashboard";
    case WEATHER_FORECAST: return "forecast";
    case WEATHER_STACKED: return "stacked";
    default: return "simple";
  }
}
