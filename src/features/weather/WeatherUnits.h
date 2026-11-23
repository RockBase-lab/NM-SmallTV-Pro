#pragma once
#include <stdint.h>

// WeatherData always stays in Celsius and km/h. Only the screen applies these
// conversions, so changing display units never changes fetching or caching.
enum WeatherTempUnit : uint8_t { WEATHER_TEMP_C = 0, WEATHER_TEMP_F = 1 };
enum WeatherWindUnit : uint8_t { WEATHER_WIND_KMH = 0, WEATHER_WIND_MS = 1, WEATHER_WIND_MPH = 2 };

static inline float weatherTempForDisplay(float celsius, uint8_t unit) {
  return unit == WEATHER_TEMP_F ? celsius * 9.0f / 5.0f + 32.0f : celsius;
}
static inline float weatherWindForDisplay(float kmh, uint8_t unit) {
  if (unit == WEATHER_WIND_MS) return kmh / 3.6f;
  if (unit == WEATHER_WIND_MPH) return kmh / 1.609344f;
  return kmh;
}
static inline const char* weatherTempLabel(uint8_t unit) {
  return unit == WEATHER_TEMP_F ? "F" : "C";
}
static inline const char* weatherWindLabel(uint8_t unit) {
  if (unit == WEATHER_WIND_MS) return "m/s";
  if (unit == WEATHER_WIND_MPH) return "mph";
  return "km/h";
}
