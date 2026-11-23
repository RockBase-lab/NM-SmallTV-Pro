#include "KoreanFont.h"
#include "font_korean10.h"
#include "font_galmuri11.h"
#include "font_galmuri12.h"
#include "font_korean16.h"
#include "Gfx.h"
#include <string.h>

// NM-TV-154 and other flash-constrained builds keep the Galmuri ASCII glyphs and
// every text/layout API, but leave the KS X 1001 Hangul bitmap tables out of the
// image. Unsupported UTF-8 code points fall back to '?'. The full Korean build
// remains the default for boards with enough flash.
#ifndef WITH_KOREAN_FONT
#define WITH_KOREAN_FONT 1
#endif

uint8_t g_koreanFontSize = 22; // 22px default (11px base x 2 scale)
uint8_t g_koreanCharGap  = 1;  // Confirmed 1px letter spacing

uint8_t utf8DecodeChar(const char* p, uint16_t* outCode) {
  if (!p || !*p) {
    if (outCode) *outCode = 0;
    return 0;
  }
  uint8_t c = (uint8_t)*p;
  if (c < 0x80) {
    if (outCode) *outCode = c;
    return 1;
  } else if ((c & 0xE0) == 0xC0) {
    if (outCode) *outCode = ((c & 0x1F) << 6) | ((uint8_t)p[1] & 0x3F);
    return 2;
  } else if ((c & 0xF0) == 0xE0) {
    if (outCode) *outCode = ((c & 0x0F) << 12) | (((uint8_t)p[1] & 0x3F) << 6) | ((uint8_t)p[2] & 0x3F);
    return 3;
  } else if ((c & 0xF8) == 0xF0) {
    if (outCode) *outCode = '?';
    return 4;
  }
  if (outCode) *outCode = '?';
  return 1;
}

#if WITH_KOREAN_FONT
static int findKoreanGlyphIndex(uint16_t unicode) {
  int low = 0, high = KOREAN_FONT_GLYPH_COUNT - 1;
  while (low <= high) {
    int mid = (low + high) / 2;
    uint16_t cp = pgm_read_word(&GALMURI12_CODEPOINTS[mid]);
    if (cp == unicode) return mid;
    if (cp < unicode) low = mid + 1;
    else high = mid - 1;
  }
  return -1;
}
#define KOREAN_GLYPH_INDEX(unicode) findKoreanGlyphIndex(unicode)
#else
// A compile-time constant lets the compiler remove every Hangul-only drawing
// branch and, with it, every reference to the large glyph arrays.
#define KOREAN_GLYPH_INDEX(unicode) (-1)
#endif

int koreanMeasureTextW(const char* utf8, uint8_t fontSize) {
  if (!utf8) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  const int asciiW = koreanGetAsciiW(fontSize);
#if WITH_KOREAN_FONT
  const int fontW  = koreanGetFontW(fontSize);
#endif

  int w = 0;
  const char* p = utf8;
  while (*p) {
    uint16_t code = 0;
    uint8_t len = utf8DecodeChar(p, &code);
    if (!len) break;
    p += len;
#if WITH_KOREAN_FONT
    if (code >= 0x20 && code <= 0x7E) {
      w += asciiW;
    } else {
      w += fontW;
    }
#else
    // Every unsupported code point is rendered as one ASCII '?'.
    w += asciiW;
#endif
  }
  return w;
}

int koreanDrawGlyph(int x, int y, uint16_t unicode, uint16_t color, int32_t bg, uint8_t fontSize) {
  Arduino_GFX* gfx = gfxDev();
  if (!gfx) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  uint8_t baseSize = (fontSize == 20 || fontSize == 10) ? 10 :
                     ((fontSize == 22 || fontSize == 11) ? 11 :
                     ((fontSize == 24 || fontSize == 12) ? 12 : 16));
  uint8_t scale = (fontSize == 20 || fontSize == 22 || fontSize == 24 || fontSize == 32) ? 2 : 1;

#if !WITH_KOREAN_FONT
  // One shared ASCII renderer avoids four copies of the same pixel loop in
  // flash-constrained builds. All base sizes and integer scaling are preserved.
  if (unicode < 0x20 || unicode > 0x7E) unicode = '?';
  const int asciiW = (baseSize == 10 ? KOREAN10_ASCII_W :
                     (baseSize == 11 ? KOREAN11_ASCII_W :
                     (baseSize == 12 ? KOREAN12_ASCII_W : KOREAN16_ASCII_W))) * scale;
  const int advW = asciiW + g_koreanCharGap;
  if (x + advW <= 0 || x >= TFT_WIDTH) return advW;

  const int asciiIdx = unicode - 0x20;
  const uint8_t* p = (baseSize == 10) ? &GALMURI10_ASCII[asciiIdx * 10] :
                     (baseSize == 11) ? &GALMURI11_ASCII[asciiIdx * 11] :
                     (baseSize == 12) ? &GALMURI12_ASCII[asciiIdx * 12] :
                                        &GALMURI16_ASCII[asciiIdx * 16];
  const int glyphW = (baseSize == 10) ? 5 : (baseSize == 16 ? 8 : 6);
  for (int r = 0; r < baseSize; r++) {
    uint8_t rowBits = pgm_read_byte(p + r);
    int py = y + r * scale;
    if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
    for (int c = 0; c < glyphW; c++) {
      int px = x + c * scale;
      if (px + scale <= 0 || px >= TFT_WIDTH) continue;
      if (rowBits & (1 << (7 - c))) {
        if (scale == 1) gfx->drawPixel(px, py, color);
        else gfx->fillRect(px, py, scale, scale, color);
      } else if (bg >= 0) {
        if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
        else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
      }
    }
  }
  if (g_koreanCharGap > 0 && bg >= 0) {
    int gapX = x + asciiW;
    if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
      gfx->fillRect(gapX, y, g_koreanCharGap, baseSize * scale, (uint16_t)bg);
    }
  }
  return advW;
#else
  const bool isAscii = (unicode >= 0x20 && unicode <= 0x7E);
  const int asciiW = (baseSize == 10 ? KOREAN10_ASCII_W :
                     (baseSize == 11 ? KOREAN11_ASCII_W :
                     (baseSize == 12 ? KOREAN12_ASCII_W : KOREAN16_ASCII_W))) * scale;
  const int fontW  = (baseSize == 10 ? KOREAN10_FONT_W :
                     (baseSize == 11 ? KOREAN11_FONT_W :
                     (baseSize == 12 ? KOREAN12_FONT_W : KOREAN16_FONT_W))) * scale;
  const int charW  = isAscii ? asciiW : fontW;
  const int advW   = charW + g_koreanCharGap;

  // Fast off-screen rejection
  if (x + advW <= 0 || x >= TFT_WIDTH) return advW;

  if (baseSize == 10) {
    // 10px base rendering (GalmuriMono9)
    if (unicode >= 0x20 && unicode <= 0x7E) {
      int asciiIdx = unicode - 0x20;
      const uint8_t* p = &GALMURI10_ASCII[asciiIdx * 10];
      for (int r = 0; r < 10; r++) {
        uint8_t rowBits = pgm_read_byte(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 5; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + asciiW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 10 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }

    const int kIdx = KOREAN_GLYPH_INDEX(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI10_GLYPHS[kIdx * 10];
      for (int r = 0; r < 10; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 10; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + fontW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 10 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }
    return koreanDrawGlyph(x, y, '?', color, bg, fontSize);
  } else if (baseSize == 11) {
    // 11px base rendering (GalmuriMono11)
    if (unicode >= 0x20 && unicode <= 0x7E) {
      int asciiIdx = unicode - 0x20;
      const uint8_t* p = &GALMURI11_ASCII[asciiIdx * 11];
      for (int r = 0; r < 11; r++) {
        uint8_t rowBits = pgm_read_byte(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 6; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + asciiW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 11 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }

    const int kIdx = KOREAN_GLYPH_INDEX(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI11_GLYPHS[kIdx * 11];
      for (int r = 0; r < 11; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 11; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + fontW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 11 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }
    return koreanDrawGlyph(x, y, '?', color, bg, fontSize);
  } else if (baseSize == 12) {
    // 12px base rendering
    if (unicode >= 0x20 && unicode <= 0x7E) {
      int asciiIdx = unicode - 0x20;
      const uint8_t* p = &GALMURI12_ASCII[asciiIdx * 12];
      for (int r = 0; r < 12; r++) {
        uint8_t rowBits = pgm_read_byte(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 6; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + asciiW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 12 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }

    const int kIdx = KOREAN_GLYPH_INDEX(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI12_GLYPHS[kIdx * 12];
      for (int r = 0; r < 12; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 12; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + fontW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 12 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }
    return koreanDrawGlyph(x, y, '?', color, bg, fontSize);
  } else {
    // 16px base rendering
    if (unicode >= 0x20 && unicode <= 0x7E) {
      int asciiIdx = unicode - 0x20;
      const uint8_t* p = &GALMURI16_ASCII[asciiIdx * 16];
      for (int r = 0; r < 16; r++) {
        uint8_t rowBits = pgm_read_byte(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 8; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + asciiW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 16 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }

    const int kIdx = KOREAN_GLYPH_INDEX(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI16_GLYPHS[kIdx * 16];
      for (int r = 0; r < 16; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py + scale <= 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 16; c++) {
          int px = x + c * scale;
          if (px + scale <= 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg >= 0) {
            if (scale == 1) gfx->drawPixel(px, py, (uint16_t)bg);
            else gfx->fillRect(px, py, scale, scale, (uint16_t)bg);
          }
        }
      }
      if (g_koreanCharGap > 0 && bg >= 0) {
        int gapX = x + fontW;
        if (gapX < TFT_WIDTH && gapX + g_koreanCharGap > 0) {
          gfx->fillRect(gapX, y, g_koreanCharGap, 16 * scale, (uint16_t)bg);
        }
      }
      return advW;
    }
    return koreanDrawGlyph(x, y, '?', color, bg, fontSize);
  }
#endif
}

int koreanDrawText(int x, int y, const char* utf8, uint16_t color, int32_t bg, uint8_t fontSize) {
  if (!utf8) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  int curX = x;
  const char* p = utf8;
  while (*p) {
    uint16_t code = 0;
    uint8_t len = utf8DecodeChar(p, &code);
    if (!len) break;
    p += len;
    if (code == '\n') break;
    curX += koreanDrawGlyph(curX, y, code, color, bg, fontSize);
  }
  return curX - x;
}

int koreanDrawTextBox(int x, int y, int maxW, int maxH, const char* utf8,
                      uint16_t color, int32_t bg, int lineGap, uint8_t fontSize) {
  if (!utf8) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  if (maxW <= 0) maxW = TFT_WIDTH - x;
  if (maxH <= 0) maxH = TFT_HEIGHT - y;

  const int fontH  = koreanGetLineH(fontSize);
  const int asciiW = koreanGetAsciiW(fontSize);
#if WITH_KOREAN_FONT
  const int fontW  = koreanGetFontW(fontSize);
#endif

  int curX = x;
  int curY = y;
  int lineHeight = fontH + lineGap;
  const char* p = utf8;

  while (*p && (curY + fontH <= y + maxH)) {
    uint16_t code = 0;
    uint8_t len = utf8DecodeChar(p, &code);
    if (!len) break;
    p += len;

    if (code == '\n') {
      curX = x;
      curY += lineHeight;
      if (curY + fontH > y + maxH) break;
      continue;
    }

#if WITH_KOREAN_FONT
    int gw = (code >= 0x20 && code <= 0x7E) ? asciiW : fontW;
#else
    // Drawing maps every unsupported code point to one ASCII '?'.
    int gw = asciiW;
#endif
    int charW = gw - g_koreanCharGap;
    if (curX + charW > x + maxW) {
      curX = x;
      curY += lineHeight;
      if (curY + fontH > y + maxH) break;
    }

    koreanDrawGlyph(curX, curY, code, color, bg, fontSize);
    curX += gw;
  }

  return curY + lineHeight - y;
}
