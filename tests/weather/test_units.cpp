// Display-only weather unit conversion. The stored/fetched values stay metric.
#include "features/weather/WeatherUnits.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failed = 0;
static void near(float actual, float expected, const char* label) {
  if (fabsf(actual - expected) > 0.051f) {
    printf("FAIL %s: %.4f != %.4f\n", label, actual, expected);
    failed++;
  }
}

int main() {
  near(weatherTempForDisplay(0, WEATHER_TEMP_C), 0, "0 C");
  near(weatherTempForDisplay(0, WEATHER_TEMP_F), 32, "0 C to F");
  near(weatherTempForDisplay(100, WEATHER_TEMP_F), 212, "100 C to F");
  near(weatherWindForDisplay(100, WEATHER_WIND_KMH), 100, "100 km/h");
  near(weatherWindForDisplay(100, WEATHER_WIND_MS), 27.8f, "100 km/h to m/s");
  near(weatherWindForDisplay(100, WEATHER_WIND_MPH), 62.1f, "100 km/h to mph");
  // Unknown values fail safe to the original metric defaults.
  near(weatherTempForDisplay(20, 99), 20, "unknown temperature unit");
  near(weatherWindForDisplay(36, 99), 36, "unknown wind unit");
  if (strcmp(weatherTempLabel(WEATHER_TEMP_F), "F") ||
      strcmp(weatherWindLabel(WEATHER_WIND_MS), "m/s") ||
      strcmp(weatherWindLabel(WEATHER_WIND_MPH), "mph")) {
    printf("FAIL unit labels\n");
    failed++;
  }
  printf("weather unit tests: %s\n", failed ? "FAILED" : "passed");
  return failed ? 1 : 0;
}
