#pragma once
#include <stdint.h>
#include <Arduino_GFX_Library.h>

#define KOREAN10_FONT_H    10
#define KOREAN10_FONT_W    10
#define KOREAN10_ASCII_W   5

#define KOREAN11_FONT_H    11
#define KOREAN11_FONT_W    11
#define KOREAN11_ASCII_W   6

#define KOREAN12_FONT_H    12
#define KOREAN12_FONT_W    12
#define KOREAN12_ASCII_W   6

#define KOREAN16_FONT_H    16
#define KOREAN16_FONT_W    16
#define KOREAN16_ASCII_W   8

// Global active Korean font size (default 22, 24, 20) and character spacing gap (default 1)
extern uint8_t g_koreanFontSize;
extern uint8_t g_koreanCharGap;

inline int koreanGetLineH(uint8_t fontSize = 0) {
  extern uint8_t g_koreanFontSize;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  if (fontSize == 20) return 20;
  if (fontSize == 22) return 22;
  if (fontSize == 24) return 24;
  if (fontSize == 32) return 32;
  if (fontSize == 10) return 10;
  if (fontSize == 11) return 11;
  if (fontSize == 12) return 12;
  return 16;
}

inline int koreanGetAsciiW(uint8_t fontSize = 0) {
  extern uint8_t g_koreanFontSize;
  extern uint8_t g_koreanCharGap;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  int w = 8;
  if (fontSize == 20) w = 10;
  else if (fontSize == 22) w = 12;
  else if (fontSize == 24) w = 12;
  else if (fontSize == 32) w = 16;
  else if (fontSize == 10) w = 5;
  else if (fontSize == 11) w = 6;
  else if (fontSize == 12) w = 6;
  return w + g_koreanCharGap;
}

inline int koreanGetFontW(uint8_t fontSize = 0) {
  extern uint8_t g_koreanFontSize;
  extern uint8_t g_koreanCharGap;
  if (fontSize == 0) fontSize = g_koreanFontSize;
  int w = 16;
  if (fontSize == 20) w = 20;
  else if (fontSize == 22) w = 22;
  else if (fontSize == 24) w = 24;
  else if (fontSize == 32) w = 32;
  else if (fontSize == 10) w = 10;
  else if (fontSize == 11) w = 11;
  else if (fontSize == 12) w = 12;
  return w + g_koreanCharGap;
}

// Decode single UTF-8 codepoint from string p, returns length in bytes (1..4)
// and writes unicode value to *outCode.
uint8_t utf8DecodeChar(const char* p, uint16_t* outCode);

// Measure pixel width of UTF-8 string with specified font size (12 or 16)
int koreanMeasureTextW(const char* utf8, uint8_t fontSize = 0);

// Draw single glyph at (x, y), returns width in pixels.
// bg < 0 means transparent; bg >= 0 draws opaque background pixel
int koreanDrawGlyph(int x, int y, uint16_t unicode, uint16_t color, int32_t bg = -1, uint8_t fontSize = 0);

// Draw single-line UTF-8 string at (x, y), returns total width drawn
int koreanDrawText(int x, int y, const char* utf8, uint16_t color, int32_t bg = -1, uint8_t fontSize = 0);

// Draw multi-line text box with automatic word/char wrapping
// Returns total height drawn in pixels.
int koreanDrawTextBox(int x, int y, int maxW, int maxH, const char* utf8,
                      uint16_t color, int32_t bg = -1, int lineGap = 4, uint8_t fontSize = 0);
