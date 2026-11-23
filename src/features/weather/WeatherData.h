#pragma once
#include <Arduino.h>
#include "WeatherForecast.h"

struct WeatherData : WeatherObservation {

  bool     valid;
  bool     error;
  uint32_t lastOkMs;
  uint32_t nextTryMs;
  uint8_t  fails;

  void clear() {
    static_cast<WeatherObservation&>(*this) = WeatherObservation{};
    valid = error = false;
    lastOkMs = nextTryMs = 0;
    fails = 0;
  }
};
