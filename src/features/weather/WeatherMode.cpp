#include "WeatherMode.h"
#include <Arduino_GFX_Library.h>
#include "../../Platform.h"
#include "../../Gfx.h"
#include "../../Clock.h"
#include "WeatherLocation.h"
#include "WeatherClient.h"
#include "WeatherClock.h"
#include <cstring>

WeatherMode g_weatherMode;

namespace {
String deviceIp() {
  IPAddress address = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP();
  return address == IPAddress(0, 0, 0, 0) ? String("") : address.toString();
}
class PanelCanvas : public WeatherCanvas {
 public:
  explicit PanelCanvas(Arduino_GFX& display) : display_(display) {}
  void rasterRow(int x, int y, const uint16_t* pixels, int count) override {
    uint16_t tinted[240];
    for (int column = 0; column < count; ++column) tinted[column] = gfxTint(pixels[column]);
    display_.draw16bitRGBBitmap(x, y, tinted, count, 1);
    if ((y & 15) == 15) yield();
  }
  void rectangle(int x, int y, int width, int height, uint16_t color, bool fill) override {
    if (fill) display_.fillRect(x, y, width, height, gfxTint(color));
    else display_.drawRect(x, y, width, height, gfxTint(color));
  }
  void circle(int x, int y, int radius, uint16_t color, bool fill) override {
    if (fill) display_.fillCircle(x, y, radius, gfxTint(color));
    else display_.drawCircle(x, y, radius, gfxTint(color));
  }
  void line(int x, int y, int endX, int endY, uint16_t color) override {
    display_.drawLine(x, y, endX, endY, gfxTint(color));
  }
  void triangle(int x, int y, int secondX, int secondY, int thirdX, int thirdY, uint16_t color) override {
    display_.fillTriangle(x, y, secondX, secondY, thirdX, thirdY, gfxTint(color));
  }
  void text(int x, int y, const char* value, unsigned size, uint16_t color) override {
    display_.setTextWrap(false);
    display_.setTextSize(size);
    display_.setTextColor(gfxTint(color));
    display_.setCursor(x, y);
    display_.print(value);
  }
 private:
  Arduino_GFX& display_;
};
}

void WeatherMode::begin(const Settings& settings) {
  weatherInit(settings);
  latitude_ = settings.clock.lat;
  longitude_ = settings.clock.lon;
  activeStyle_ = settings.weatherStyle;
  dashboardStartedMs_ = millis();
  nextStyleMs_ = millis() + uint32_t(settings.weatherRotateSec) * 1000UL;
  needRender_ = true;
}

void WeatherMode::invalidate(const Settings& settings) {
  if (latitude_ != settings.clock.lat || longitude_ != settings.clock.lon) {
    weatherInit(settings);
    latitude_ = settings.clock.lat;
    longitude_ = settings.clock.lon;
  }
  activeStyle_ = settings.weatherStyle;
  dashboardStartedMs_ = millis();
  nextStyleMs_ = millis() + uint32_t(settings.weatherRotateSec) * 1000UL;
  needRender_ = true;
}

void WeatherMode::wake(const Settings& settings) {
  g_abortFetch = false;
  dashboardStartedMs_ = millis();
  nextStyleMs_ = millis() + uint32_t(settings.weatherRotateSec) * 1000UL;
  weatherDeferNextFetch(800);
  needRender_ = true;
  clockReapply(settings, true);
}

void WeatherMode::render(const Settings& settings) {
  Arduino_GFX* display = gfxDev();
  if (!display) return;
  const WeatherData& data = weatherGet();
  WeatherClockView view;
  view.weather = data;
  view.weatherValid = data.valid;
  view.error = data.error;
  view.clockValid = clockNow(view.time);
  view.style = activeStyle_;
  view.tempUnit = settings.weatherTempUnit;
  view.windUnit = settings.weatherWindUnit;
  const String address = deviceIp();
  view.ipAddress = address.c_str();
  view.elapsedMs = millis() - dashboardStartedMs_;
  const String& location = settings.clock.locationName;
  bool ascii = location.length() > 0;
  for (size_t index = 0; index < location.length(); ++index)
    if (uint8_t(location[index]) < 32 || uint8_t(location[index]) > 126) ascii = false;
  view.location = ascii ? location.c_str() : "Weather";
  PanelCanvas canvas(*display);
  renderWeatherClock(canvas, view);
  renderedSpacemanFrame_ = weatherSpacemanFrame(view.elapsedMs);
  formatWeatherDashboardInfo(view, renderedDashboardInfo_, sizeof(renderedDashboardInfo_));
  dashboardNetworkMs_ = millis();
  renderedMinute_ = view.clockValid ? uint32_t(time(nullptr) / 60) : 0;
  renderedSecond_ = view.clockValid ? view.time.tm_sec : -1;
  renderedLastOk_ = data.lastOkMs;
  renderedError_ = data.error;
}

void WeatherMode::service(const Settings& settings) {
  if (settings.weatherAuto && int32_t(millis() - nextStyleMs_) >= 0) {
    activeStyle_ = (activeStyle_ + 1) % 4;
    dashboardStartedMs_ = millis();
    nextStyleMs_ = millis() + uint32_t(settings.weatherRotateSec) * 1000UL;
    needRender_ = true;
  }
  if (activeStyle_ != WEATHER_STACKED && !weatherLocationSet(settings.clock.lat, settings.clock.lon)) {
    if (needRender_) gfxMessage("WEATHER", "Set location in Weather", C_WHITE);
    needRender_ = false;
    return;
  }
  if (needRender_) { render(settings); needRender_ = false; }
  if (activeStyle_ != WEATHER_STACKED) weatherService(settings);
  const WeatherData& data = weatherGet();
  tm current{};
  const bool clockValid = clockNow(current);
  const int second = clockValid ? current.tm_sec : -1;
  uint32_t minute = clockValid ? uint32_t(time(nullptr) / 60) : 0;
  if (data.lastOkMs != renderedLastOk_ || data.error != renderedError_ || minute != renderedMinute_) {
    render(settings);
  } else if (activeStyle_ == WEATHER_DASHBOARD && second != renderedSecond_ && gfxDev()) {
    PanelCanvas canvas(*gfxDev());
    renderWeatherSeconds(canvas, current.tm_sec, clockValid);
    renderedSecond_ = second;
  }
  if (activeStyle_ == WEATHER_DASHBOARD && gfxDev()) {
    const uint32_t now = millis(), elapsed = now - dashboardStartedMs_;
    const uint8_t frame = weatherSpacemanFrame(elapsed);
    PanelCanvas canvas(*gfxDev());
    if (frame != renderedSpacemanFrame_) {
      renderWeatherSpaceman(canvas, frame);
      renderedSpacemanFrame_ = frame;
    }
    if (now - dashboardNetworkMs_ >= 1000) {
      dashboardNetworkMs_ = now;
      const String address = deviceIp();
      WeatherClockView view;
      view.weather = data;
      view.weatherValid = data.valid;
      view.ipAddress = address.c_str();
      view.elapsedMs = elapsed;
      char info[sizeof(renderedDashboardInfo_)];
      formatWeatherDashboardInfo(view, info, sizeof(info));
      if (std::strcmp(info, renderedDashboardInfo_)) {
        renderWeatherDashboardInfo(canvas, view);
        std::strcpy(renderedDashboardInfo_, info);
      }
    }
  }
}
