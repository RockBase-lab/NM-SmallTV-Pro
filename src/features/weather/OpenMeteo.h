#pragma once
#include "WeatherForecast.h"
#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <cstring>

inline int weatherDateWeekday(const char* date) {
  if (!date || std::strlen(date) != 10 || date[4] != '-' || date[7] != '-') return -1;
  for (unsigned index = 0; index < 10; ++index)
    if (index != 4 && index != 7 && (date[index] < '0' || date[index] > '9')) return -1;
  int year = 0, month = 0, day = 0;
  if (std::sscanf(date, "%d-%d-%d", &year, &month, &day) != 3 || year < 1970 || year > 2200 || month < 1 || month > 12) return -1;
  const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
  if (day < 1 || day > days[month - 1] + (month == 2 && leap ? 1 : 0)) return -1;
  const int offsets[] = {0,3,2,5,0,3,5,1,4,6,2,4};
  if (month < 3) --year;
  return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

inline bool weatherNumber(JsonVariantConst value, float minimum, float maximum, float& result) {
  if (!value.is<float>() || value.is<bool>()) return false;
  float candidate = value.as<float>();
  if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum) return false;
  result = candidate;
  return true;
}

inline void weatherSunTime(JsonVariantConst value, char (&target)[6]) {
  const char* text = value.as<const char*>();
  if (!text || std::strlen(text) < 16 || text[10] != 'T' || text[13] != ':') return;
  for (unsigned index : {11u, 12u, 14u, 15u}) if (text[index] < '0' || text[index] > '9') return;
  if ((text[11] - '0') * 10 + text[12] - '0' > 23 || (text[14] - '0') * 10 + text[15] - '0' > 59) return;
  std::memcpy(target, text + 11, 5);
  target[5] = 0;
}

inline bool parseOpenMeteo(JsonVariantConst root, WeatherObservation& observation) {
  WeatherObservation candidate;
  JsonVariantConst current = root["current"];
  float humidity = 0, code = 0, day = 0;
  if (!weatherNumber(current["temperature_2m"], -150, 100, candidate.temp) ||
      !weatherNumber(current["relative_humidity_2m"], 0, 100, humidity) ||
      !weatherNumber(current["weather_code"], 0, 99, code) ||
      !weatherNumber(current["is_day"], 0, 1, day) ||
      !weatherNumber(current["wind_speed_10m"], 0, 500, candidate.windSpeed)) return false;
  if (std::floor(code) != code || std::floor(humidity) != humidity || std::floor(day) != day) return false;
  candidate.humidity = static_cast<uint8_t>(humidity);
  candidate.weatherCode = static_cast<uint8_t>(code);
  candidate.isDay = day != 0;
  JsonVariantConst daily = root["daily"];
  for (unsigned index = 0; index < 3; ++index) {
    WeatherDay& forecast = candidate.forecast[index];
    const char* date = daily["time"][index].as<const char*>();
    if (weatherDateWeekday(date) < 0 ||
        !weatherNumber(daily["weather_code"][index], 0, 99, code) || std::floor(code) != code ||
        !weatherNumber(daily["temperature_2m_min"][index], -150, 100, forecast.tempMin) ||
        !weatherNumber(daily["temperature_2m_max"][index], -150, 100, forecast.tempMax) || forecast.tempMin > forecast.tempMax) return false;
    if (index && std::strcmp(candidate.forecast[index - 1].date, date) >= 0) return false;
    std::memcpy(forecast.date, date, 11);
    forecast.weatherCode = static_cast<uint8_t>(code);
  }
  candidate.forecastCount = 3;
  candidate.tempMin = candidate.forecast[0].tempMin;
  candidate.tempMax = candidate.forecast[0].tempMax;
  weatherSunTime(daily["sunrise"][0], candidate.sunrise);
  weatherSunTime(daily["sunset"][0], candidate.sunset);
  observation = candidate;
  return true;
}
