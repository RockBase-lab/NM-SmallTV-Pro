#include "WeatherClient.h"
#include "../../Platform.h"
#include <ArduinoJson.h>
#include "OpenMeteo.h"
#include "WeatherLocation.h"
#include <memory>
#include <algorithm>
#include <new>

static WeatherData g_weather;
static bool g_fetching = false;

const char* weatherCodeToDesc(uint8_t code) {
  switch (code) {
    case 0:  return "Clear";
    case 1:  return "Mainly Clear";
    case 2:  return "Partly Cloudy";
    case 3:  return "Overcast";
    case 45:
    case 48: return "Foggy";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 56:
    case 57: return "Freezing Drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 66:
    case 67: return "Freezing Rain";
    case 71:
    case 73:
    case 75: return "Snow";
    case 77: return "Snow Grains";
    case 80:
    case 81:
    case 82: return "Rain Showers";
    case 85:
    case 86: return "Snow Showers";
    case 95:
    case 96:
    case 99: return "Thunderstorm";
    default: return "Cloudy";
  }
}

void weatherInit(const Settings& s) {
  g_weather.clear();
  g_weather.nextTryMs = millis();
  g_fetching = false;
}

void weatherForceRefresh() {
  g_weather.nextTryMs = millis();
  g_weather.fails = 0;
  g_fetching = false;
}

void weatherDeferNextFetch(uint32_t delayMs) {
  uint32_t target = millis() + delayMs;
  if ((int32_t)(target - g_weather.nextTryMs) > 0) {
    g_weather.nextTryMs = target;
  }
  g_fetching = false;
}

const WeatherData& weatherGet() {
  return g_weather;
}

static bool fetchOpenMeteo(float lat, float lon, uint16_t timeoutMs, WeatherData& data) {
  if (g_abortFetch) return false;

  char url[512];
  snprintf(url, sizeof(url), "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,is_day,weather_code,wind_speed_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset"
           "&forecast_days=3&timezone=auto&temperature_unit=celsius&wind_speed_unit=kmh", lat, lon);

  std::unique_ptr<SecureClient> client(platformMakeSecureClient(4096));
  if (!client) return false;

  HTTPClient http;
  uint16_t t = timeoutMs < 1000 ? 1000 : (timeoutMs > 8000 ? 8000 : timeoutMs);
  http.setTimeout(t);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(*client, url)) return false;
  http.setUserAgent(F("SmallTV/Open-Meteo"));
  http.addHeader("Accept", "application/json");

  yield();
  int code = http.GET();
  yield();
  if (code != HTTP_CODE_OK || g_abortFetch) {
    http.end();
    return false;
  }

  constexpr size_t limit = 4096;
  const int expected = http.getSize();
  if (expected > static_cast<int>(limit)) { http.end(); return false; }
  std::unique_ptr<char[]> payload(new(std::nothrow) char[limit + 1]);
  if (!payload) { http.end(); return false; }
  Stream& stream = http.getStream();
  size_t used = 0;
  uint32_t started = millis();
  bool complete = false;
  while (!g_abortFetch && uint32_t(millis() - started) < t) {
    int available = stream.available();
    if (available > 0) {
      size_t count = std::min<size_t>(available, limit - used);
      if (!count) break;
      used += stream.readBytes(payload.get() + used, count);
    }
    if ((expected >= 0 && used == static_cast<size_t>(expected)) ||
        (expected < 0 && !http.connected() && !stream.available())) { complete = true; break; }
    delay(1);
  }
  http.end();
  if (!complete || g_abortFetch || !used) return false;
  payload[used] = 0;
  JsonDocument document;
  if (deserializeJson(document, payload.get(), used)) return false;
  WeatherObservation candidate;
  if (!parseOpenMeteo(document.as<JsonVariantConst>(), candidate)) return false;
  static_cast<WeatherObservation&>(data) = candidate;
  return true;
}

void weatherService(const Settings& s) {
  if (!weatherLocationSet(s.clock.lat, s.clock.lon)) return;
  if (g_fetching) return;
  if ((int32_t)(millis() - g_weather.nextTryMs) < 0) return;

  float lat = s.clock.lat;
  float lon = s.clock.lon;

  uint32_t pollInterval = (uint32_t)(s.weatherPollMin ? s.weatherPollMin : 10) * 60000UL;

  g_abortFetch = false;
  g_fetching = true;
  if (fetchOpenMeteo(lat, lon, s.httpTimeout, g_weather)) {
    g_weather.valid = true;
    g_weather.error = false;
    g_weather.lastOkMs = millis();
    g_weather.fails = 0;
    g_weather.nextTryMs = millis() + pollInterval;
  } else {
    g_weather.error = true;
    if (g_weather.fails < 100) g_weather.fails++;
    uint32_t back = 15000UL; // 15 sec backoff
    uint8_t shifts = (g_weather.fails > 6) ? 6 : (g_weather.fails - 1);
    back <<= shifts; // max 15000 << 6 = 960,000 ms (~16 min), NO OVERFLOW!
    if (back > pollInterval) back = pollInterval;
    g_weather.nextTryMs = millis() + back;
  }
  g_fetching = false;
}
