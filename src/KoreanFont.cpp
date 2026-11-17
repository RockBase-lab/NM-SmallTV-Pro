#include "KoreanFont.h"
#include "font_korean10.h"
#include "font_galmuri12.h"
#include "font_korean16.h"
#include "Gfx.h"
#include <string.h>

uint8_t g_koreanFontSize = 20; // Default to 20px (10px x 2) for 12-char testing

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

int koreanMeasureTextW(const char* utf8, uint8_t fontSize) {
  if (!utf8) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  const int asciiW = koreanGetAsciiW(fontSize);
  const int fontW  = koreanGetFontW(fontSize);

  int w = 0;
  const char* p = utf8;
  while (*p) {
    uint16_t code = 0;
    uint8_t len = utf8DecodeChar(p, &code);
    if (!len) break;
    p += len;
    if (code >= 0x20 && code <= 0x7E) {
      w += asciiW;
    } else {
      w += fontW;
    }
  }
  return w;
}

int koreanDrawGlyph(int x, int y, uint16_t unicode, uint16_t color, uint16_t bg, uint8_t fontSize) {
  Arduino_GFX* gfx = gfxDev();
  if (!gfx) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  uint8_t baseSize = (fontSize == 20 || fontSize == 10) ? 10 :
                     ((fontSize == 24 || fontSize == 12) ? 12 : 16);
  uint8_t scale = (fontSize == 20 || fontSize == 24 || fontSize == 32) ? 2 : 1;

  if (baseSize == 10) {
    // 10px base rendering (GalmuriMono9)
    if (unicode >= 0x20 && unicode <= 0x7E) {
      int asciiIdx = unicode - 0x20;
      const uint8_t* p = &GALMURI10_ASCII[asciiIdx * 10];
      for (int r = 0; r < 10; r++) {
        uint8_t rowBits = pgm_read_byte(p + r);
        int py = y + r * scale;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 5; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN10_ASCII_W * scale; // 5 * scale
    }

    int kIdx = findKoreanGlyphIndex(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI10_GLYPHS[kIdx * 10];
      for (int r = 0; r < 10; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 10; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN10_FONT_W * scale; // 10 * scale (20 when scale=2)
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
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 6; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN12_ASCII_W * scale; // 6 * scale
    }

    int kIdx = findKoreanGlyphIndex(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI12_GLYPHS[kIdx * 12];
      for (int r = 0; r < 12; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 12; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN12_FONT_W * scale; // 12 * scale
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
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 8; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (7 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN16_ASCII_W * scale; // 8 * scale
    }

    int kIdx = findKoreanGlyphIndex(unicode);
    if (kIdx >= 0) {
      const uint16_t* p = &GALMURI16_GLYPHS[kIdx * 16];
      for (int r = 0; r < 16; r++) {
        uint16_t rowBits = pgm_read_word(p + r);
        int py = y + r * scale;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        for (int c = 0; c < 16; c++) {
          int px = x + c * scale;
          if (px < 0 || px >= TFT_WIDTH) continue;
          if (rowBits & (1 << (15 - c))) {
            if (scale == 1) gfx->drawPixel(px, py, color);
            else gfx->fillRect(px, py, scale, scale, color);
          } else if (bg != 0x0000 || color == 0x0000) {
            if (scale == 1) gfx->drawPixel(px, py, bg);
            else gfx->fillRect(px, py, scale, scale, bg);
          }
        }
      }
      return KOREAN16_FONT_W * scale; // 16 * scale
    }
    return koreanDrawGlyph(x, y, '?', color, bg, fontSize);
  }
}

int koreanDrawText(int x, int y, const char* utf8, uint16_t color, uint16_t bg, uint8_t fontSize) {
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
                      uint16_t color, uint16_t bg, int lineGap, uint8_t fontSize) {
  if (!utf8) return 0;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  if (maxW <= 0) maxW = TFT_WIDTH - x;
  if (maxH <= 0) maxH = TFT_HEIGHT - y;

  const int fontH  = koreanGetLineH(fontSize);
  const int asciiW = koreanGetAsciiW(fontSize);
  const int fontW  = koreanGetFontW(fontSize);

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
      continue;
    }

    int gw = (code >= 0x20 && code <= 0x7E) ? asciiW : fontW;
    if (curX + gw > x + maxW) {
      curX = x;
      curY += lineHeight;
      if (curY + fontH > y + maxH) break;
    }

    koreanDrawGlyph(curX, curY, code, color, bg, fontSize);
    curX += gw;
  }

  return curY + lineHeight - y;
}
