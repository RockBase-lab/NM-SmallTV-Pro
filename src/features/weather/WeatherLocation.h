#pragma once
#include <cmath>

inline bool weatherLocationSet(float latitude, float longitude) {
  return std::isfinite(latitude) && std::isfinite(longitude) &&
         latitude >= -90 && latitude <= 90 && longitude >= -180 && longitude <= 180 &&
         (latitude != 0 || longitude != 0);
}
