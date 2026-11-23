#include "OpenMeteo.h"
#include <cassert>
#include <cstring>
#include <iostream>

static const char* response = R"({"current":{"temperature_2m":0,"relative_humidity_2m":0,"is_day":0,"weather_code":3,"wind_speed_10m":0},"daily":{"time":["2026-11-20","2026-11-21","2026-11-22"],"weather_code":[3,0,2],"temperature_2m_min":[-1,-1,-2],"temperature_2m_max":[8,5,5],"sunrise":["2026-11-20T07:04","2026-11-21T07:05","2026-11-22T07:06"],"sunset":["2026-11-20T16:55","2026-11-21T16:54","2026-11-22T16:54"]}})";

int main() {
  JsonDocument document;
  assert(!deserializeJson(document, response));
  WeatherObservation observation{};
  assert(parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  assert(observation.temp == 0 && observation.humidity == 0 && observation.windSpeed == 0);
  assert(!observation.isDay && observation.weatherCode == 3 && observation.forecastCount == 3);
  assert(observation.forecast[2].tempMin == -2 && observation.forecast[0].tempMax == 8);
  assert(std::strcmp(observation.forecast[0].date, "2026-11-20") == 0);
  assert(std::strcmp(observation.sunrise, "07:04") == 0);
  assert(std::strcmp(observation.sunset, "16:55") == 0);
  assert(weatherDateWeekday("2026-11-20") == 5);
  assert(weatherDateWeekday("2024-02-29") == 4);
  for (const char* invalid : {"2026-02-29", "2026-13-01", "2026-11-31", "bad"})
    assert(weatherDateWeekday(invalid) == -1);
  document["current"]["temperature_2m"] = nullptr;
  observation.temp = 37;
  assert(!parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  assert(observation.temp == 37 && observation.forecastCount == 3);
  for (const char* field : {"temperature_2m", "relative_humidity_2m", "is_day", "weather_code", "wind_speed_10m"}) {
    assert(!deserializeJson(document, response));
    document["current"].remove(field);
    assert(!parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  }
  for (const char* field : {"time", "weather_code", "temperature_2m_min", "temperature_2m_max"}) {
    assert(!deserializeJson(document, response));
    document["daily"][field][1] = nullptr;
    assert(!parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  }
  assert(!deserializeJson(document, response));
  document["daily"]["time"][1] = "2026-02-30";
  assert(!parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  assert(!deserializeJson(document, response));
  document["current"]["relative_humidity_2m"] = 101;
  assert(!parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  assert(!deserializeJson(document, response));
  document["daily"]["sunrise"][0] = nullptr;
  assert(parseOpenMeteo(document.as<JsonVariantConst>(), observation));
  assert(std::strcmp(observation.sunrise, "--:--") == 0);
  std::cout << "Open-Meteo parser tests passed\n";
}
