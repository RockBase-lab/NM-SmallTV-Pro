#include "WeatherClient.h"
#include "../../Platform.h"
#include <ArduinoJson.h>

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
    case 61:
    case 63:
    case 65: return "Rain";
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

static bool fetchOpenMeteo(float lat, float lon, uint16_t timeoutMs, WeatherData& d) {
  if (g_abortFetch) return false;

  // Use plain HTTP for ultra-fast, zero-TLS-overhead fetch
  String url = F("http://api.open-meteo.com/v1/forecast?latitude=");
  url += String(lat, 4);
  url += F("&longitude=");
  url += String(lon, 4);
  url += F("&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min&timezone=auto");

  std::unique_ptr<WiFiClient> client(new WiFiClient());
  if (!client) return false;

  HTTPClient http;
  uint16_t t = (timeoutMs > 3000) ? 3000 : timeoutMs;
  http.setTimeout(t);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(*client, url)) return false;
  http.setUserAgent(F(FW_NAME));
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  if (code != HTTP_CODE_OK || g_abortFetch) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument filter;
  filter["current"]["temperature_2m"] = true;
  filter["current"]["relative_humidity_2m"] = true;
  filter["current"]["weather_code"] = true;
  filter["current"]["wind_speed_10m"] = true;
  filter["daily"]["temperature_2m_max"] = true;
  filter["daily"]["temperature_2m_min"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload.c_str(), DeserializationOption::Filter(filter));
  if (err) return false;

  JsonObjectConst cur = doc["current"];
  if (cur.isNull()) return false;

  d.temp = cur["temperature_2m"].as<float>();
  d.humidity = cur["relative_humidity_2m"].as<uint8_t>();
  d.weatherCode = cur["weather_code"].as<uint8_t>();
  d.windSpeed = cur["wind_speed_10m"].as<float>();

  d.tempMax = doc["daily"]["temperature_2m_max"][0] | d.temp;
  d.tempMin = doc["daily"]["temperature_2m_min"][0] | d.temp;

  d.valid = true;
  d.error = false;
  d.lastOkMs = millis();
  return true;
}

void weatherService(const Settings& s) {
  if (g_fetching) return;
  if ((int32_t)(millis() - g_weather.nextTryMs) < 0) return;

  float lat = s.clock.lat;
  float lon = s.clock.lon;
  if (lat == 0.0f && lon == 0.0f) {
    lat = 37.566f;
    lon = 126.9784f; // Default Seoul
  }

  g_abortFetch = false;
  g_fetching = true;
  if (fetchOpenMeteo(lat, lon, s.httpTimeout, g_weather)) {
    g_weather.valid = true;
    g_weather.error = false;
    g_weather.lastOkMs = millis();
    g_weather.fails = 0;
    g_weather.nextTryMs = millis() + 600000UL; // 10 minutes interval
  } else {
    g_weather.error = true;
    if (g_weather.fails < 250) g_weather.fails++;
    uint32_t back = 15000UL; // 15 sec backoff
    for (uint8_t i = 1; i < g_weather.fails; i++) back <<= 1;
    if (back > 600000UL) back = 600000UL;
    g_weather.nextTryMs = millis() + back;
  }
  g_fetching = false;
}
