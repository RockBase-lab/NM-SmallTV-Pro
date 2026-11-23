#pragma once
#include "../../Mode.h"
#include "../../config.h"

class WeatherMode : public DisplayMode {
 public:
  const char* id() const override { return "weather"; }
  uint8_t     modeConst() const override { return MODE_WEATHER; }

  void begin(const Settings& s) override;
  void service(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override;

 private:
  void render(const Settings& s);

  uint32_t renderedLastOk_ = 0xFFFFFFFF;
  bool     renderedError_ = false;
  bool     needRender_ = true;
  float latitude_ = 999;
  float longitude_ = 999;
  uint32_t renderedMinute_ = 0xffffffff;
  int renderedSecond_ = -1;
  uint8_t activeStyle_ = 0;
  uint32_t nextStyleMs_ = 0;
  uint32_t dashboardStartedMs_ = 0;
  uint32_t dashboardNetworkMs_ = 0;
  uint8_t renderedSpacemanFrame_ = 255;
  char renderedDashboardInfo_[40] = {};
};

extern WeatherMode g_weatherMode;
