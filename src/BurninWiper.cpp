#include "BurninWiper.h"
#include "Gfx.h"
#include "Clock.h"
#include <Arduino_GFX_Library.h>
#include "features/notify/notify_frames.h"

static bool     g_burninRunning = false;
static uint32_t g_burninStartMs = 0;
static uint32_t g_burninDurationMs = 0;
static uint32_t g_lastFrameMs = 0;
static uint32_t g_lastTriggerMin = 0xFFFFFFFF;
static uint32_t g_lastIntervalMs = 0;
static String   g_currentEffect = "matrix";
static bool     g_effectInitialized = false;
static uint32_t g_frame = 0;

// Procedural effect state
struct MatrixCol {
  int y;
  int speed;
  int len;
};
static MatrixCol g_matrixCols[24];

struct Star3D {
  int16_t x, y, z;
  int16_t prevX, prevY;
  uint8_t prevSize;
};
static Star3D g_stars[70];

void burninInit() {
  g_burninRunning = false;
  g_lastIntervalMs = millis();
  struct tm now;
  if (clockNow(now)) {
    g_lastTriggerMin = ((uint32_t)now.tm_yday * 1440) + ((uint32_t)now.tm_hour * 60) + now.tm_min;
  }
}

bool burninActive() {
  return g_burninRunning;
}

uint32_t burninHeldMs() {
  if (!g_burninStartMs) return 0;
  return millis() - g_burninStartMs;
}

void burninDismiss() {
  g_burninRunning = false;
  g_effectInitialized = false;
}

static void initEffect(const String& eff) {
  Arduino_GFX* gfx = gfxDev();
  if (!gfx) return;

  if (eff == "black") {
    gfx->fillScreen(C_BLACK);
  } else if (eff == "white") {
    gfx->fillScreen(C_WHITE);
  } else if (eff == "matrix") {
    gfx->fillScreen(C_BLACK);
    for (int i = 0; i < 24; i++) {
      g_matrixCols[i].y = -(rand() % 240);
      g_matrixCols[i].speed = 3 + (rand() % 5);
      g_matrixCols[i].len = 8 + (rand() % 12);
    }
  } else if (eff == "starfield") {
    gfx->fillScreen(C_BLACK);
    for (int i = 0; i < 70; i++) {
      g_stars[i].x = (rand() % 240) - 120;
      g_stars[i].y = (rand() % 240) - 120;
      g_stars[i].z = 1 + (rand() % 255);
      g_stars[i].prevX = -1;
      g_stars[i].prevY = -1;
      g_stars[i].prevSize = 0;
    }
  } else if (eff == "rainbow") {
    // Redrawn frame by frame
  } else if (eff == "snow") {
    // Redrawn frame by frame
  } else if (eff == "mascot") {
    gfx->fillScreen(C_BLACK);
  }
  g_effectInitialized = true;
  g_frame = 0;
  g_lastFrameMs = millis();
}

static void stepRainbow(Arduino_GFX* gfx) {
  static const uint16_t c[8] = {
    0xFFFF, // White
    0xFFE0, // Yellow
    0x07FF, // Cyan
    0x07E0, // Green
    0xF81F, // Magenta
    0xF800, // Red
    0x001F, // Blue
    0x0000  // Black
  };
  int offset = (g_frame * 3) % 240;
  for (int i = 0; i < 8; i++) {
    int x = (i * 30 + offset) % 240;
    if (x + 30 <= 240) {
      gfx->fillRect(x, 0, 30, 240, c[i]);
    } else {
      gfx->fillRect(x, 0, 240 - x, 240, c[i]);
      gfx->fillRect(0, 0, 30 - (240 - x), 240, c[i]);
    }
  }
}

static void stepMatrix(Arduino_GFX* gfx) {
  for (int i = 0; i < 24; i++) {
    int tailY = g_matrixCols[i].y - g_matrixCols[i].len * 8;
    if (tailY >= 0 && tailY < 240) {
      gfx->fillRect(i * 10, tailY, 10, g_matrixCols[i].speed + 8, C_BLACK);
    }
    int headY = g_matrixCols[i].y;
    if (headY >= 0 && headY < 232) {
      gfx->setTextColor(0xFFFF, 0x0000);
      gfx->setTextSize(1);
      gfx->setCursor(i * 10 + 2, headY);
      char ch = (char)(33 + (rand() % 90));
      gfx->print(ch);
    }
    int gY = headY - 8;
    if (gY >= 0 && gY < 232) {
      gfx->setTextColor(0x07E0, 0x0000);
      gfx->setTextSize(1);
      gfx->setCursor(i * 10 + 2, gY);
      char ch = (char)(33 + (rand() % 90));
      gfx->print(ch);
    }
    g_matrixCols[i].y += g_matrixCols[i].speed;
    if (g_matrixCols[i].y - g_matrixCols[i].len * 8 > 240) {
      g_matrixCols[i].y = -(rand() % 40);
      g_matrixCols[i].speed = 3 + (rand() % 5);
      g_matrixCols[i].len = 8 + (rand() % 12);
    }
  }
}

static void stepStarfield(Arduino_GFX* gfx) {
  for (int i = 0; i < 70; i++) {
    if (g_stars[i].prevX >= 0) {
      if (g_stars[i].prevSize <= 1) {
        gfx->drawPixel(g_stars[i].prevX, g_stars[i].prevY, C_BLACK);
      } else {
        gfx->fillRect(g_stars[i].prevX, g_stars[i].prevY, g_stars[i].prevSize, g_stars[i].prevSize, C_BLACK);
      }
    }
    g_stars[i].z -= 5;
    if (g_stars[i].z <= 4) {
      g_stars[i].x = (rand() % 240) - 120;
      g_stars[i].y = (rand() % 240) - 120;
      g_stars[i].z = 255;
      g_stars[i].prevX = -1;
      continue;
    }
    int px = 120 + ((int32_t)g_stars[i].x * 128) / g_stars[i].z;
    int py = 120 + ((int32_t)g_stars[i].y * 128) / g_stars[i].z;
    if (px < 0 || px >= 240 || py < 0 || py >= 240) {
      g_stars[i].x = (rand() % 240) - 120;
      g_stars[i].y = (rand() % 240) - 120;
      g_stars[i].z = 255;
      g_stars[i].prevX = -1;
      continue;
    }
    uint16_t color;
    uint8_t sz;
    if (g_stars[i].z > 160)      { color = 0x632C; sz = 1; }
    else if (g_stars[i].z > 70)  { color = 0xAD55; sz = 1; }
    else if (g_stars[i].z > 25)  { color = 0xFFFF; sz = 1; }
    else                         { color = 0xFFFF; sz = 2; }

    if (sz <= 1) {
      gfx->drawPixel(px, py, color);
    } else {
      gfx->fillRect(px, py, sz, sz, color);
    }
    g_stars[i].prevX = px;
    g_stars[i].prevY = py;
    g_stars[i].prevSize = sz;
  }
}

static void stepSnow(Arduino_GFX* gfx) {
  static const uint16_t noiseColors[4] = { 0x0000, 0x4208, 0x8410, 0xFFFF };
  for (int y = 0; y < 240; y += 6) {
    for (int x = 0; x < 240; x += 6) {
      gfx->fillRect(x, y, 6, 6, noiseColors[rand() & 3]);
    }
  }
}

static void stepMascot(Arduino_GFX* gfx) {
  const NotifyAnim& a = notify_anims[0]; // NOTIFY_ANIM_DONE
  uint16_t f = g_frame % a.frame_count;
  const uint8_t* p = a.frames + (size_t)f * (a.w * a.h);
  int scale = 4;
  int w = a.w * scale;
  int h = a.h * scale;
  int x0 = (240 - w) / 2;
  int y0 = (240 - h) / 2;

  for (int dy = 0; dy < a.h; dy++) {
    for (int dx = 0; dx < a.w; dx++) {
      uint8_t ci = pgm_read_byte(p + dy * a.w + dx);
      uint16_t col = (ci < NOTIFY_PALETTE_SIZE) ? a.palette[ci] : 0x0000;
      gfx->fillRect(x0 + dx * scale, y0 + dy * scale, scale, scale, col);
    }
  }
}

static void renderFrame() {
  Arduino_GFX* gfx = gfxDev();
  if (!gfx) return;

  if (g_currentEffect == "rainbow") {
    stepRainbow(gfx);
  } else if (g_currentEffect == "matrix") {
    stepMatrix(gfx);
  } else if (g_currentEffect == "starfield") {
    stepStarfield(gfx);
  } else if (g_currentEffect == "snow") {
    stepSnow(gfx);
  } else if (g_currentEffect == "mascot") {
    stepMascot(gfx);
  }
}

static void burninStart(const char* effect, uint16_t durationSec) {
  g_currentEffect = effect ? effect : "matrix";
  g_burninDurationMs = (uint32_t)durationSec * 1000UL;
  if (g_burninDurationMs < 1000UL) g_burninDurationMs = 1000UL;
  g_burninStartMs = millis();
  g_burninRunning = true;
  g_effectInitialized = false;
  initEffect(g_currentEffect);
}

void burninTriggerPreview(const char* effect, uint16_t durationSec) {
  burninStart(effect, durationSec);
}

void burninService(const Settings& s) {
  if (g_burninRunning) {
    if (millis() - g_burninStartMs >= g_burninDurationMs) {
      burninDismiss();
      return;
    }
    if (!g_effectInitialized) {
      initEffect(g_currentEffect);
    }
    uint32_t now = millis();
    uint32_t interval = 40;
    if (g_currentEffect == "starfield") interval = 30;
    else if (g_currentEffect == "matrix") interval = 45;
    else if (g_currentEffect == "snow") interval = 60;
    else if (g_currentEffect == "mascot") interval = 100;
    else if (g_currentEffect == "black" || g_currentEffect == "white") interval = 500;

    if (now - g_lastFrameMs >= interval) {
      g_lastFrameMs = now;
      g_frame++;
      renderFrame();
    }
    return;
  }

  // Not running: check schedule
  if (!s.burnin.enabled) return;

  uint32_t now = millis();

  if (s.burnin.triggerType == "time") {
    struct tm t;
    if (clockNow(t)) {
      int targetH = 12, targetM = 0;
      if (s.burnin.timeStr.length() >= 3) {
        sscanf(s.burnin.timeStr.c_str(), "%d:%d", &targetH, &targetM);
      }
      if (t.tm_hour == targetH && t.tm_min == targetM) {
        uint32_t minKey = ((uint32_t)t.tm_yday * 1440) + ((uint32_t)t.tm_hour * 60) + t.tm_min;
        if (minKey != g_lastTriggerMin) {
          g_lastTriggerMin = minKey;
          burninStart(s.burnin.effect.c_str(), s.burnin.durationSec);
        }
      }
    }
  } else {
    // "interval" mode (default)
    uint16_t interval = s.burnin.intervalMin ? s.burnin.intervalMin : 60;
    struct tm t;
    if (clockNow(t)) {
      uint32_t totalMins = (uint32_t)t.tm_hour * 60 + t.tm_min;
      if (totalMins % interval == 0) {
        uint32_t minKey = ((uint32_t)t.tm_yday * 1440) + totalMins;
        if (minKey != g_lastTriggerMin) {
          g_lastTriggerMin = minKey;
          burninStart(s.burnin.effect.c_str(), s.burnin.durationSec);
        }
      }
    } else {
      // Fallback without clock sync
      if (now - g_lastIntervalMs >= (uint32_t)interval * 60000UL) {
        g_lastIntervalMs = now;
        burninStart(s.burnin.effect.c_str(), s.burnin.durationSec);
      }
    }
  }
}
