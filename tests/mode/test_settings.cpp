// The "mode" key of a save or an import. A save that does not name a mode the
// build knows must leave the stored mode alone. Before, an empty or unknown
// text turned the mode into the stock ticker. Built with the stand-in core
// headers of the fuzz target (see tests/mode/run.sh).
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "Settings.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>

FSClass LittleFS;
EspClass ESP;

static int g_failed = 0, g_checks = 0;
#define CHECK(cond, ...)                                                   \
  do {                                                                     \
    g_checks++;                                                            \
    if (!(cond)) {                                                         \
      g_failed++;                                                          \
      printf("FAIL %s:%d: %s  ", __FILE__, __LINE__, #cond);               \
      printf(__VA_ARGS__);                                                 \
      printf("\n");                                                        \
    }                                                                      \
  } while (0)

static void apply(Settings& s, const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) { printf("bad test json: %s\n", json); g_failed++; return; }
  settingsApplyJson(s, doc.as<JsonObjectConst>());
}

// Start from a device that stored `mode`, then apply a save.
static uint8_t after(uint8_t stored, const char* json) {
  Settings s;
  s.setDefaults();
  s.mode = stored;
  apply(s, json);
  return s.mode;
}

static void testNamesSetTheMode() {
  CHECK(after(MODE_STOCKS, "{\"mode\":\"theme\"}") == MODE_THEME, "theme");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"radar\"}") == MODE_RADAR, "radar");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"usage\"}") == MODE_USAGE, "usage");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"ha\"}") == MODE_HA, "ha");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"carousel\"}") == MODE_CAROUSEL, "carousel");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"timer\"}") == MODE_TIMER, "timer");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"Timer\"}") == MODE_TIMER, "timer case ignored");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"crypto\"}") == MODE_CRYPTO, "crypto");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"pictures\"}") == MODE_PICTURES, "pictures");
  CHECK(after(MODE_STOCKS, "{\"mode\":\"picture\"}") == MODE_PICTURES, "picture alias");
  CHECK(after(MODE_RADAR, "{\"mode\":\"stocks\"}") == MODE_STOCKS, "stocks");
  CHECK(after(MODE_RADAR, "{\"mode\":\"Theme\"}") == MODE_THEME, "case is ignored");
}

static void testEmptyMode() {
  // The page sends "" when its mode list lacks the stored mode (a build
  // without that feature). The stored mode stays.
  CHECK(after(MODE_RADAR, "{\"mode\":\"\"}") == MODE_RADAR, "radar kept");
  CHECK(after(MODE_THEME, "{\"mode\":\"\"}") == MODE_THEME, "theme kept");
  CHECK(after(MODE_HA, "{\"mode\":\"\"}") == MODE_HA, "ha kept");
  CHECK(after(MODE_CAROUSEL, "{\"mode\":\"\"}") == MODE_CAROUSEL, "carousel kept");
}

static void testUnknownMode() {
  CHECK(after(MODE_USAGE, "{\"mode\":\"nonsense\"}") == MODE_USAGE, "unknown text kept");
  CHECK(after(MODE_USAGE, "{\"mode\":\"theme2\"}") == MODE_USAGE, "near miss kept");
}

static void testNoModeOrOtherType() {
  CHECK(after(MODE_HA, "{\"brightness\":40}") == MODE_HA, "no mode key");
  CHECK(after(MODE_HA, "{\"mode\":7}") == MODE_HA, "number");
  CHECK(after(MODE_HA, "{\"mode\":null}") == MODE_HA, "null");
}

static void testPartialSave() {
  Settings s;
  s.setDefaults();
  apply(s, "{\"mode\":\"radar\",\"hostname\":\"smalltv-test\"}");
  CHECK(s.mode == MODE_RADAR, "stored radar");
  apply(s, "{\"mode\":\"\",\"hostname\":\"smalltv-test-2\"}");
  CHECK(s.mode == MODE_RADAR, "partial save with empty mode keeps radar");
  CHECK(s.hostname == "smalltv-test-2", "the rest of the save applies");
}

static void testCryptoDefaultsAndRoundTrip() {
  Settings s;
  s.setDefaults();
  CHECK(s.crypto.symbolCount == 3, "three default pairs");
  CHECK(!strcmp(s.crypto.symbols[0].symbol, "BTCUSDT"), "BTC default");
  CHECK(!strcmp(s.crypto.symbols[1].symbol, "ETHUSDT"), "ETH default");
  CHECK(!strcmp(s.crypto.symbols[2].symbol, "SOLUSDT"), "SOL default");
  CHECK(s.crypto.symbols[0].source == SRC_BINANCE, "Binance default source");
  CHECK(s.crypto.chartStyle == "line", "line chart default");
  CHECK(s.crypto.rotateSec == 10, "ten second rotation default");
  CHECK(!s.carouselCrypto, "crypto is opt-in for the app carousel");
  CHECK(&tickerSettings(s, 2) == &s.crypto, "set 2 selects crypto settings");

  // A config written before Crypto existed has no crypto key: defaults survive.
  apply(s, "{\"hostname\":\"old-config\"}");
  CHECK(s.crypto.symbolCount == 3, "old config keeps default pairs");
  CHECK(s.crypto.symbols[0].source == SRC_BINANCE, "old config keeps Binance");

  apply(s, "{\"mode\":\"crypto\",\"carouselCrypto\":false,\"crypto\":{\"range\":\"5m\",\"rotateSec\":17,\"chartStyle\":\"candle\",\"symbols\":[{\"symbol\":\"dogeusdt\",\"name\":\"Dogecoin\",\"source\":\"binance\"}]}}");
  CHECK(s.mode == MODE_CRYPTO, "crypto mode applies");
  CHECK(!s.carouselCrypto, "carousel flag applies");
  CHECK(s.crypto.symbolCount == 1 && s.crypto.rotateSec == 17, "crypto slice applies");
  CHECK(s.crypto.range == "5m" && s.crypto.chartStyle == "candle", "chart settings apply");
  CHECK(s.crypto.symbols[0].source == SRC_BINANCE, "Binance source parses");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  s.crypto.toJson(root["crypto"].to<JsonObject>());
  root["mode"] = "crypto";
  root["carouselCrypto"] = s.carouselCrypto;
  Settings restored;
  restored.setDefaults();
  settingsApplyJson(restored, root);
  CHECK(restored.mode == MODE_CRYPTO && !restored.carouselCrypto, "mode and carousel round-trip");
  CHECK(restored.crypto.symbolCount == 1, "symbol count round-trip");
  CHECK(!strcmp(restored.crypto.symbols[0].symbol, "dogeusdt"), "custom pair round-trip");
  CHECK(restored.crypto.symbols[0].source == SRC_BINANCE, "Binance source round-trip");
  CHECK(restored.crypto.range == "5m" && restored.crypto.rotateSec == 17, "chart cadence round-trip");
}

static void testPictureDefaultsValidationAndRoundTrip() {
  Settings s;
  s.setDefaults();
  CHECK(s.pictureId == "ultra-01", "first fullscreen picture is the default");
  CHECK(s.pictureAuto, "picture rotation defaults on");
  CHECK(s.pictureRotateSec == 10, "ten second picture rotation default");
  CHECK(!s.carouselPictures, "pictures are opt-in for the app carousel");

  // Configurations written before Pictures existed keep these defaults.
  apply(s, "{\"hostname\":\"old-config\"}");
  CHECK(s.pictureId == "ultra-01" && s.pictureAuto && s.pictureRotateSec == 10,
        "old config keeps picture defaults");

  apply(s, "{\"mode\":\"pictures\",\"pictureId\":\"picture-room\",\"pictureAuto\":false,\"pictureRotateSec\":17,\"carouselPictures\":true}");
  CHECK(s.mode == MODE_PICTURES && s.pictureId == "picture-room", "picture selection applies");
  CHECK(!s.pictureAuto && s.pictureRotateSec == 17 && s.carouselPictures,
        "picture cadence and carousel flag apply");

  apply(s, "{\"pictureId\":\"../bad\",\"pictureRotateSec\":1}");
  CHECK(s.pictureId == "picture-room", "unsafe picture ID is rejected");
  CHECK(s.pictureRotateSec == 2, "picture cadence minimum is clamped");
  apply(s, "{\"pictureId\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"pictureRotateSec\":5000}");
  CHECK(s.pictureId == "picture-room", "overlong picture ID is rejected");
  CHECK(s.pictureRotateSec == 3600, "picture cadence maximum is clamped");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  settingsToJson(s, root, false);
  Settings restored;
  restored.setDefaults();
  settingsApplyJson(restored, root);
  CHECK(restored.mode == MODE_PICTURES && restored.pictureId == "picture-room", "picture mode round-trip");
  CHECK(!restored.pictureAuto && restored.pictureRotateSec == 3600, "picture cadence round-trip");
  CHECK(restored.carouselPictures, "picture carousel flag round-trip");
}

static void testWeatherClockSettings() {
  Settings s;
  s.setDefaults();
  CHECK(s.weatherStyle == 0 && !s.weatherAuto && s.weatherRotateSec == 20, "weather clock defaults");
  apply(s, "{\"weatherStyle\":\"forecast\",\"weatherAuto\":true,\"weatherRotateSec\":1}");
  CHECK(s.weatherStyle == 2 && s.weatherAuto && s.weatherRotateSec == 10, "style and minimum dwell");
  apply(s, "{\"weatherStyle\":\"unknown\",\"weatherRotateSec\":5000}");
  CHECK(s.weatherStyle == 2 && s.weatherRotateSec == 3600, "unknown style retained and dwell clamped");
  apply(s, "{\"weatherStyle\":null,\"weatherAuto\":1}");
  CHECK(s.weatherStyle == 2 && s.weatherAuto, "invalid field types ignored");
  for (unsigned style = 0; style < 4; ++style) {
    s.weatherStyle = style;
    JsonDocument doc;
    settingsToJson(s, doc.to<JsonObject>(), false);
    Settings restored;
    restored.setDefaults();
    settingsApplyJson(restored, doc.as<JsonObjectConst>());
    CHECK(restored.weatherStyle == style && restored.weatherAuto && restored.weatherRotateSec == 3600, "weather clock round-trip");
  }
}

int main() {
  Settings defaults;
  defaults.setDefaults();
  CHECK(defaults.carouselTicker && defaults.carouselWeather, "ready-to-use apps included");
  CHECK(!defaults.carouselTicker2 && !defaults.carouselUsage && !defaults.carouselRadar && !defaults.carouselHa,
        "integrations needing setup are opt-in");
  JsonDocument emptyBackup;
  settingsToJson(defaults, emptyBackup.to<JsonObject>(), true);
  Settings replacement;
  replacement.setDefaults();
  replacement.auth.pass = "later-password";
  replacement.wg.privateKey = "later-private-key";
  replacement.ha.brokerPass = "later-mqtt-password";
  replacement.setDefaults();
  settingsApplyJson(replacement, emptyBackup.as<JsonObjectConst>(), true);
  CHECK(replacement.auth.pass.length() == 0 && replacement.wg.privateKey.length() == 0 && replacement.ha.brokerPass.length() == 0,
        "full backup restoration clears credentials absent from the backup");
  for (const char* name : {"stocks", "stocks2", "crypto", "weather", "usage", "radar", "ha", "theme", "pictures", "carousel", "timer"}) {
    Settings saved;
    saved.setDefaults();
    JsonDocument selection;
    selection["mode"] = name;
    selection["weatherStyle"] = "forecast";
    settingsApplyJson(saved, selection.as<JsonObjectConst>());
    JsonDocument storage;
    settingsToJson(saved, storage.to<JsonObject>(), false);
    Settings restored;
    restored.setDefaults();
    settingsApplyJson(restored, storage.as<JsonObjectConst>(), true);
    CHECK(restored.mode == saved.mode && restored.weatherStyle == saved.weatherStyle, "saved mode survives startup");
  }
  testNamesSetTheMode();
  testEmptyMode();
  testUnknownMode();
  testNoModeOrOtherType();
  testPartialSave();
  testCryptoDefaultsAndRoundTrip();
  testPictureDefaultsValidationAndRoundTrip();
  testWeatherClockSettings();
  printf("%d checks, %d failed\n", g_checks, g_failed);
  return g_failed ? 1 : 0;
}
