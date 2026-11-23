#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "Settings.h"
#include <cassert>
#include <cstring>
#include <cstdio>

FSClass LittleFS;
EspClass ESP;

static void checkPresets(const Settings& settings) {
  const TickerSettings& ticker = settings.ticker;
  assert(ticker.symbolCount == 3);
  assert(!strcmp(ticker.symbols[0].symbol, "005930"));
  assert(!strcmp(ticker.symbols[0].name, "Samsung"));
  assert(ticker.symbols[0].source == SRC_NAVER);
  assert(!strcmp(ticker.symbols[1].symbol, "NVDA"));
  assert(!strcmp(ticker.symbols[1].name, "NVIDIA"));
  assert(ticker.symbols[1].source == SRC_YAHOO);
  assert(!strcmp(ticker.symbols[2].symbol, "AAPL"));
  assert(!strcmp(ticker.symbols[2].name, "Apple"));
  assert(ticker.symbols[2].source == SRC_YAHOO);
  assert(ticker.chartStyle == "line" && ticker.range == "1mo");
  assert(ticker.points == 20 && ticker.showChart && ticker.showPrice);
  assert(ticker.rotateSec == 10 && ticker.pollSec == 120);
  assert(ticker.webhookUrl.length() == 0);
  for (unsigned index = 0; index < ticker.symbolCount; ++index)
    assert(ticker.symbols[index].qty == 0 && ticker.symbols[index].cost == 0);
}

static void apply(Settings& settings, const char* json, bool restoring = false) {
  JsonDocument document;
  assert(!deserializeJson(document, json));
  settingsApplyJson(settings, document.as<JsonObjectConst>(), restoring);
}

int main() {
  Settings settings;
  settings.setDefaults();
  checkPresets(settings);
  assert(settings.ticker2.symbolCount == 0);
  assert(settings.crypto.symbolCount == 3 && settings.crypto.symbols[0].source == SRC_BINANCE);

  apply(settings, R"({"ticker":{"symbols":[],"points":0,"showChart":false,"chartStyle":"candle","range":"1d"},"hostname":"old-device","clock":{"lat":37.5,"lon":127}})", true);
  checkPresets(settings);
  assert(settings.hostname == "old-device" && settings.clock.lat == 37.5f);

  apply(settings, R"({"ticker":{"symbols":[]}})");
  assert(settings.ticker.symbolCount == 0);
  JsonDocument saved;
  settingsToJson(settings, saved.to<JsonObject>(), true);
  assert(saved["stockDefaultsVersion"].as<int>() == 1);
  Settings restored;
  restored.setDefaults();
  settingsApplyJson(restored, saved.as<JsonObjectConst>(), true);
  assert(restored.ticker.symbolCount == 0);

  restored.setDefaults();
  apply(restored, R"({"symbols":[],"range":"1d"})", true);
  checkPresets(restored);

  apply(restored, R"({"ticker":{"symbols":[{"symbol":"MSFT","name":"Mine","source":"yahoo","qty":2,"cost":123}],"chartStyle":"candle","range":"5d","points":15,"showChart":false,"pollSec":300}})", true);
  assert(restored.ticker.symbolCount == 1);
  assert(!strcmp(restored.ticker.symbols[0].symbol, "MSFT"));
  assert(restored.ticker.symbols[0].qty == 2 && restored.ticker.symbols[0].cost == 123);
  assert(restored.ticker.chartStyle == "candle" && restored.ticker.range == "5d");
  assert(restored.ticker.points == 15 && !restored.ticker.showChart && restored.ticker.pollSec == 300);

  apply(restored, R"({"stockDefaultsVersion":1,"ticker":{"symbols":[]}})", true);
  assert(restored.ticker.symbolCount == 0);
  apply(restored, R"({"brightness":35})");
  assert(restored.ticker.symbolCount == 0 && restored.brightness == 35);

  settings.setDefaults();
  settingsToJson(settings, saved.to<JsonObject>(), true);
  settingsApplyJson(restored, saved.as<JsonObjectConst>(), true);
  checkPresets(restored);
  puts("Stock defaults, upgrade, custom settings and intentional deletion passed");
}
