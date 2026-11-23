#include "WeatherClock.h"
#include "WeatherUnits.h"
#include "OpenMeteo.h"
#include "WeatherDigits.h"
#include "WeatherFont.h"
#include "WeatherSpaceman.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr uint16_t black = 0x0000, white = 0xffff, yellow = 0xffe0;
constexpr uint16_t cyan = 0x07ff, green = 0x07e0, gray = 0x8410;
const char* const weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

uint8_t fontByte(const uint8_t* address) {
#ifdef ARDUINO
  return pgm_read_byte(address);
#else
  return *address;
#endif
}
uint16_t coverageColor(uint16_t color, unsigned coverage, uint16_t background = black) {
  unsigned red = (((color >> 11) * coverage) + ((background >> 11) * (3 - coverage))) / 3;
  unsigned greenValue = ((((color >> 5) & 63) * coverage) + (((background >> 5) & 63) * (3 - coverage))) / 3;
  unsigned blue = (((color & 31) * coverage) + ((background & 31) * (3 - coverage))) / 3;
  return (red << 11) | (greenValue << 5) | blue;
}
int lettering(WeatherCanvas& canvas, int x, int y, const char* value, int height, uint16_t color, int available, uint16_t background = black) {
  int naturalWidth = 0;
  for (const char* character = value; *character; ++character) {
    unsigned code = static_cast<unsigned char>(*character);
    if (code < 32 || code > 126) code = '?';
    naturalWidth += fontByte(&weatherLetters[code - 32].advance);
  }
  if (naturalWidth) height = std::max(12, std::min(height, available * 24 / naturalWidth));
  for (const char* character = value; *character; ++character) {
    unsigned code = static_cast<unsigned char>(*character);
    if (code < 32 || code > 126) code = '?';
    WeatherLetter glyph;
#ifdef ARDUINO
    memcpy_P(&glyph, weatherLetters + code - 32, sizeof(glyph));
#else
    glyph = weatherLetters[code - 32];
#endif
    int width = std::max(1, glyph.width * height / 24), advance = std::max(1, glyph.advance * height / 24);
    if (width > available) break;
    uint8_t samples[240];
    weatherfont::unpack(weatherLetterPixels + glyph.offset,samples,glyph.width * 24 / 4);
    uint16_t pixels[64];
    for (int row = 0; row < height; ++row) {
      for (int column = 0; column < width; ++column) {
        unsigned index = (row * 24 / height) * glyph.width + column * glyph.width / width;
        unsigned coverage = (samples[index / 4] >> (6 - (index % 4) * 2)) & 3;
        pixels[column] = coverageColor(color, coverage, background);
      }
      canvas.rasterRow(x, y + row, pixels, width);
    }
    x += advance;available -= advance;
  }
  return x;
}

void label(WeatherCanvas& canvas, int x, int y, const char* value, unsigned size, uint16_t color, int width = 240) {
  unsigned length = std::strlen(value);
  while (size > 1 && length * 6 * size > static_cast<unsigned>(width)) --size;
  char bounded[41];
  unsigned count = std::min<unsigned>(length, std::min<unsigned>(width / (6 * size), 40));
  std::memcpy(bounded, value, count);
  bounded[count] = 0;
  if (size == 1) canvas.text(x, y, bounded, size, color);
  else lettering(canvas, x, y, bounded, size * 10, color, width);
}

void horizontal(WeatherCanvas& canvas, int x, int y, int width, int thickness, uint16_t color) {
  canvas.rectangle(x + thickness / 2, y, width - thickness, thickness, color, true);
  canvas.triangle(x, y + thickness / 2, x + thickness / 2, y, x + thickness / 2, y + thickness, color);
  canvas.triangle(x + width, y + thickness / 2, x + width - thickness / 2, y, x + width - thickness / 2, y + thickness, color);
}

void digit(WeatherCanvas& canvas, int x, int y, int width, int height, int value, uint16_t color, bool italic = false) {
  if (value < 0) { horizontal(canvas, x, y + height / 2, width, 4, color); return; }
  uint16_t offset;
#ifdef ARDUINO
  offset=pgm_read_word(weatherDigitOffsets + value % 10);
#else
  offset=weatherDigitOffsets[value % 10];
#endif
  uint8_t samples[800];
  weatherfont::unpack(weatherDigits+offset,samples,sizeof(samples));
  uint16_t pixels[64];
  for (int row = 0; row < height; ++row) {
    int shift = italic ? (height - row) * 6 / height : 0;
    for (int column = 0; column < width; ++column) {
      unsigned index = (row * 80 / height) * 40 + column * 40 / width;
      unsigned coverage = (samples[index / 4] >> (6 - (index % 4) * 2)) & 3;
      pixels[column] = coverageColor(color, coverage);
    }
    canvas.rasterRow(x + shift, y + row, pixels, width);
  }
}

void pair(WeatherCanvas& canvas, int x, int y, int width, int height, int value, uint16_t color, bool italic = false) {
  digit(canvas, x, y, width, height, value < 0 ? -1 : value / 10, color, italic);
  digit(canvas, x + width + 5, y, width, height, value < 0 ? -1 : value % 10, color, italic);
}

void clockLine(WeatherCanvas& canvas, int x, int y, int width, int height, const WeatherClockView& view) {
  bool italic = view.style == WEATHER_SIMPLE;
  pair(canvas, x, y, width, height, view.clockValid ? view.time.tm_hour : -1, white, italic);
  int colonX = x + 2 * width + 12;
  canvas.rectangle(colonX, y + height / 3, 5, 5, white, true);
  canvas.rectangle(colonX, y + height * 2 / 3, 5, 5, white, true);
  pair(canvas, colonX + 12, y, width, height, view.clockValid ? view.time.tm_min : -1, yellow, italic);
}

void rounded(WeatherCanvas& canvas, int x, int y, int width, int height, int radius, uint16_t color, bool fill) {
  if (width <= 0 || height <= 0) return;
  radius = std::min(radius, (std::min(width, height) - 1) / 2);
  if (fill) {
    canvas.rectangle(x + radius, y, width - 2 * radius, height, color, true);
    canvas.rectangle(x, y + radius, width, height - 2 * radius, color, true);
    for (int side : {0, 1}) for (int bottom : {0, 1})
      canvas.circle(x + (side ? width - radius - 1 : radius), y + (bottom ? height - radius - 1 : radius), radius, color, true);
  } else {
    canvas.line(x + radius, y, x + width - radius - 1, y, color);
    canvas.line(x + radius, y + height - 1, x + width - radius - 1, y + height - 1, color);
    canvas.line(x, y + radius, x, y + height - radius - 1, color);
    canvas.line(x + width - 1, y + radius, x + width - 1, y + height - radius - 1, color);
    for (int row = 0; row < radius; ++row) {
      int edge = radius - std::lround(std::sqrt(radius * radius - (radius - row) * (radius - row)));
      canvas.rectangle(x + edge, y + row, 1, 1, color, true);
      canvas.rectangle(x + width - edge - 1, y + row, 1, 1, color, true);
      canvas.rectangle(x + edge, y + height - row - 1, 1, 1, color, true);
      canvas.rectangle(x + width - edge - 1, y + height - row - 1, 1, 1, color, true);
    }
  }
}

const char* condition(uint8_t code) {
  if (code == 0) return "Clear";
  if (code <= 3) return "Clouds";
  if (code <= 48) return "Fog";
  if (code <= 57) return "Drizzle";
  if (code <= 67 || (code >= 80 && code <= 82)) return "Rain";
  if (code <= 86) return "Snow";
  return "Storm";
}

void icon(WeatherCanvas& canvas, int x, int y, int size, uint8_t code, bool day) {
  int centerX = x + size / 2, centerY = y + size / 2;
  if (code <= 2) {
    canvas.circle(centerX, centerY, size / 4, day ? yellow : white, true);
    if (day) {
      for (int angle = 0; angle < 8; ++angle) {
        double radians = angle * 0.785398163;
        canvas.line(centerX + std::cos(radians) * size * .32, centerY + std::sin(radians) * size * .32,
                    centerX + std::cos(radians) * size * .43, centerY + std::sin(radians) * size * .43, yellow);
      }
    } else canvas.circle(centerX + size / 9, centerY - size / 9, size / 4, black, true);
    if (code == 0) return;
  }
  int cloudY = y + size * 11 / 20;
  for (int row = size / 6; row <= size * 3 / 4; ++row) {
    uint16_t pixels[96];
    int first = size, last = 0;
    for (int column = 0; column < size; ++column) {
      auto inside = [column, row](int center, int top, int radius) {
        return (column-center)*(column-center)+(row-top)*(row-top)<=radius*radius;
      };
      bool cloud = inside(size/4,size*11/20,size/6) || inside(size/2,size*17/40,size/4) ||
          inside(size*3/4,size*11/20,size/6) ||
          (column >= size/4 && column <= size*3/4 && row >= size*11/20 && row <= size*7/10);
      if (cloud) {
        first = std::min(first,column);last = column;
        unsigned shade = std::min(255, 210 + (size * 3 / 4 - row) * 50 / size);
        pixels[column] = ((shade>>3)<<11)|((shade>>2)<<5)|(std::min(255u,shade+9)>>3);
      } else pixels[column] = black;
    }
    if (last >= first) canvas.rasterRow(x+first,y+row,pixels+first,last-first+1);
  }
  if (code == 45 || code == 48) {
    canvas.line(x + 4, y + size * 4 / 5, x + size - 4, y + size * 4 / 5, gray);
    canvas.line(x + 8, y + size * 9 / 10, x + size - 8, y + size * 9 / 10, gray);
  } else if (code >= 51) {
    for (int drop = 0; drop < 3; ++drop) {
      int dropX = x + size * (drop + 1) / 4;
      if ((code >= 71 && code <= 77) || code == 85 || code == 86)
        canvas.circle(dropX, y + size * 5 / 6, 2, cyan, true);
      else canvas.line(dropX, y + size * 4 / 5, dropX - 2, y + size * 9 / 10, cyan);
    }
    if (code >= 95) canvas.triangle(centerX, cloudY, centerX - size / 8, y + size, centerX + size / 8, y + size * 3 / 4, yellow);
  }
}

void temperature(char* out, size_t count, float value, uint8_t unit) {
  std::snprintf(out, count, "%.0f%c", weatherTempForDisplay(value, unit), unit == WEATHER_TEMP_F ? 'F' : 'C');
}

}

uint8_t weatherSpacemanFrame(uint32_t elapsedMs) {
  unsigned phase = elapsedMs % WeatherSpacemanDuration;
  for (unsigned frame = 0; frame < WeatherSpacemanFrames; ++frame) {
#ifdef ARDUINO
    unsigned deadline = pgm_read_word(weatherSpacemanDeadlines + frame);
#else
    unsigned deadline = weatherSpacemanDeadlines[frame];
#endif
    if (phase < deadline) return frame;
  }
  return 0;
}

void renderWeatherSpaceman(WeatherCanvas& canvas, uint8_t frame) {
  frame %= WeatherSpacemanFrames;
#ifdef ARDUINO
  unsigned offset = pgm_read_word(weatherSpacemanOffsets + frame);
#else
  unsigned offset = weatherSpacemanOffsets[frame];
#endif
  uint16_t pixels[WeatherSpacemanSize];
  uint8_t packed[WeatherSpacemanSize * WeatherSpacemanSize / 8];
  weatherfont::unpack(weatherSpacemanPixels + offset, packed, sizeof(packed));
  for (unsigned row = 0; row < WeatherSpacemanSize; ++row) {
    for (unsigned column = 0; column < WeatherSpacemanSize; ++column) {
      unsigned index = row * WeatherSpacemanSize + column;
      pixels[column] = packed[index / 8] & (1 << (7 - index % 8)) ? white : black;
    }
    canvas.rasterRow(180, 180 + row, pixels, WeatherSpacemanSize);
  }
}

void formatWeatherDashboardInfo(const WeatherClockView& view, char* output, size_t size) {
  if ((view.elapsedMs / 10000) % 2)
    std::snprintf(output, size, "IP %s", view.ipAddress && *view.ipAddress ? view.ipAddress : "offline");
  else std::snprintf(output, size, "Sunrise %s", view.weatherValid ? view.weather.sunrise : "--:--");
}

void renderWeatherDashboardInfo(WeatherCanvas& canvas, const WeatherClockView& view) {
  char text[40];
  formatWeatherDashboardInfo(view, text, sizeof(text));
  canvas.rectangle(6, 34, 162, 24, black, true);
  lettering(canvas, 6, 34, text, 21, white, 162);
}

void renderWeatherSeconds(WeatherCanvas& canvas, int seconds, bool valid) {
  canvas.rectangle(177, 104, 55, 40, black, true);
  pair(canvas, 181, 105, 19, 36, valid ? seconds : -1, white);
}

void renderWeatherClock(WeatherCanvas& canvas, const WeatherClockView& view) {
  canvas.rectangle(0, 0, 240, 240, black, true);
  const WeatherObservation& weather = view.weather;
  char text[64];
  if (view.style == WEATHER_STACKED) {
    pair(canvas, 10, 8, 53, 106, view.clockValid ? view.time.tm_hour : -1, white);
    pair(canvas, 119, 126, 53, 106, view.clockValid ? view.time.tm_min : -1, yellow);
    if (!view.clockValid) label(canvas, 8, 116, "Syncing time...", 1, gray);
    return;
  }
  if (view.style == WEATHER_FORECAST) {
    if (view.clockValid) std::snprintf(text, sizeof(text), "Forecast %02d:%02d", view.time.tm_hour, view.time.tm_min);
    else std::snprintf(text, sizeof(text), "Forecast --:--");
    lettering(canvas, 39, 2, text, 26, yellow, 198);
    for (unsigned index = 0; index < 3; ++index) {
      int top = 32 + index * 68;
      rounded(canvas, 3, top, 234, 61, 7, cyan, false);
      if (!view.weatherValid || index >= weather.forecastCount) {
        label(canvas, 18, top + 23, view.error ? "Weather unavailable" : "Loading forecast...", 1, gray);
        continue;
      }
      const WeatherDay& forecast = weather.forecast[index];
      int weekday = weatherDateWeekday(forecast.date);
      lettering(canvas, 12, top + 6, weekday < 0 ? "---" : weekdays[weekday], 23, yellow, 69);
      int month = 0, day = 0;
      std::sscanf(forecast.date + 5, "%d-%d", &month, &day);
      std::snprintf(text, sizeof(text), "%02d/%s", day, month >= 1 && month <= 12 ? months[month - 1] : "---");
      lettering(canvas, 12, top + 38, text, 18, green, 74);
      icon(canvas, 82, top + 5, 54, forecast.weatherCode, true);
      std::snprintf(text, sizeof(text), "%.0f/%.0f", weatherTempForDisplay(forecast.tempMin, view.tempUnit), weatherTempForDisplay(forecast.tempMax, view.tempUnit));
      int end = lettering(canvas, 143, top + 29, text, 25, white, 79);
      canvas.circle(std::min(end + 3, 230), top + 33, 2, white, false);
      lettering(canvas, 145, top + 5, condition(forecast.weatherCode), 17, cyan, 80);
    }
    label(canvas, 5, 238 - 8, "Open-Meteo", 1, gray, 100);
  } else if (view.style == WEATHER_DASHBOARD) {
    lettering(canvas, 6, 4, view.location, 24, yellow, 152);
    renderWeatherDashboardInfo(canvas, view);
    if (view.weatherValid) icon(canvas, 172, 0, 62, weather.weatherCode, weather.isDay);
    rounded(canvas, 175, 66, 60, 25, 4, white, true);
    lettering(canvas, 179, 69, view.weatherValid ? condition(weather.weatherCode) : "---", 18, 0x03df, 52, white);
    clockLine(canvas, 6, 66, 29, 76, view);
    renderWeatherSeconds(canvas, view.time.tm_sec, view.clockValid);
    if (view.clockValid) std::snprintf(text, sizeof(text), "%02d/%02d/%04d", view.time.tm_mday, view.time.tm_mon + 1, view.time.tm_year + 1900);
    else std::snprintf(text, sizeof(text), "--/--/---- ---");
    lettering(canvas, 6, 149, text, 23, white, 165);
    lettering(canvas, 164, 149, view.clockValid ? weekdays[view.time.tm_wday] : "---", 23, green, 69);
    if (view.weatherValid) {
      temperature(text, sizeof(text), weather.temp, view.tempUnit);
      label(canvas, 113, 184, text, 2, white, 66);
      std::snprintf(text, sizeof(text), "%u%%", weather.humidity);
      label(canvas, 113, 216, text, 2, white, 66);
      rounded(canvas, 29, 188, 76, 12, 5, white, false);
      rounded(canvas, 32, 191, std::max(0, std::min(70, int(weather.temp + 20))), 6, 2, cyan, true);
      rounded(canvas, 29, 220, 76, 12, 5, white, false);
      rounded(canvas, 32, 223, weather.humidity * 70 / 100, 6, 2, green, true);
    } else label(canvas, 8, 190, view.error ? "Weather unavailable" : "Loading weather...", 1, gray, 168);
    canvas.circle(13, 196, 4, white, false);
    canvas.rectangle(11, 180, 4, 15, white, false);
    canvas.triangle(13, 211, 7, 223, 19, 223, cyan);
    canvas.circle(13, 224, 6, cyan, true);
    renderWeatherSpaceman(canvas, weatherSpacemanFrame(view.elapsedMs));
  } else {
    if (view.weatherValid) {
      icon(canvas, 12, 16, 96, weather.weatherCode, weather.isDay);
      temperature(text, sizeof(text), weather.temp, view.tempUnit);
      lettering(canvas, 138, 29, text, 50, white, 98);
      label(canvas, 122, 93, condition(weather.weatherCode), 2, yellow, 114);
    } else label(canvas, 18, 57, view.error ? "Weather unavailable" : "Loading weather...", 1, gray);
    clockLine(canvas, 17, 148, 42, 76, view);
    if (view.weatherValid) {
      std::snprintf(text, sizeof(text), "Wind %.0f %s", weatherWindForDisplay(weather.windSpeed, view.windUnit), weatherWindLabel(view.windUnit));
      label(canvas, 18, 231, text, 1, gray, 216);
    }
  }
  if (view.error) canvas.circle(234, 6, 3, 0xf800, true);
}
