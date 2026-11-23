#include "config.h"
#if WITH_THEME
#include "JpegPicture.h"
#if defined(CONFIG_IDF_TARGET_ESP32)
#include <esp32/rom/tjpgd.h>
#include "Gfx.h"
#include <Arduino_GFX_Library.h>
#include <algorithm>
#include <memory>
#include <new>

namespace {
struct Input {
  smalltv::Source& source;
  uint32_t offset, length, position = 0;
  Input(smalltv::Source& input, uint32_t start, uint32_t count) : source(input), offset(start), length(count) {}
};
UINT readJpeg(JDEC* decoder, BYTE* buffer, UINT requested) {
  auto& input = *static_cast<Input*>(decoder->device);
  const UINT count = std::min<uint32_t>(requested, input.length - input.position);
  if (buffer && !input.source.read(input.offset + input.position, buffer, count)) return 0;
  input.position += count;
  return count;
}
UINT paintJpeg(JDEC*, void* bitmap, JRECT* region) {
  if (region->right >= 240 || region->bottom >= 240 || region->left > region->right || region->top > region->bottom) return 0;
  const auto* pixels = static_cast<const uint8_t*>(bitmap);
  uint16_t row[16];
  const unsigned width = region->right - region->left + 1;
  if (width > 16) return 0;
  for (unsigned y = region->top; y <= region->bottom; ++y) {
    for (unsigned x = 0; x < width; ++x) {
      row[x] = gfxTint(((pixels[0] & 248) << 8) | ((pixels[1] & 252) << 3) | (pixels[2] >> 3));
      pixels += 3;
    }
    gfxDev()->draw16bitRGBBitmap(region->left, y, row, width, 1);
  }
  yield();
  return 1;
}
}
bool drawJpegPicture(smalltv::Source& source, uint32_t offset, uint32_t length) {
  std::unique_ptr<uint8_t[]> workspace(new (std::nothrow) uint8_t[4096]);
  if (!workspace) return false;
  Input input(source, offset, length);
  JDEC decoder{};
  return jd_prepare(&decoder, readJpeg, workspace.get(), 4096, &input) == JDR_OK &&
         decoder.width == 240 && decoder.height == 240 && jd_decomp(&decoder, paintJpeg, 0) == JDR_OK;
}
#else
bool drawJpegPicture(smalltv::Source&, uint32_t, uint32_t) { return false; }
#endif
#endif
