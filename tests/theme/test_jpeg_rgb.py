import os
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('src/features/theme/JpegPicture.cpp').read_text(encoding='utf-8')
paint = re.search(r'UINT paintJpeg\([\s\S]*?\n\}', source).group()
fixture = r'''
#include <cassert>
#include <cstdint>
#include <vector>
using UINT = unsigned;
struct JDEC {};
struct JRECT { unsigned left, right, top, bottom; };
std::vector<uint16_t> rendered;
unsigned yields = 0;
uint16_t gfxTint(uint16_t color) { return color; }
void yield() { ++yields; }
struct Display {
  void draw16bitRGBBitmap(unsigned x, unsigned y, uint16_t* row, unsigned width, unsigned height) {
    assert(x == 7 && y == 11 + rendered.size() / 3 && width == 3 && height == 1);
    rendered.insert(rendered.end(), row, row + width);
  }
} display;
Display* gfxDev() { return &display; }
'''
checks = r'''
int main() {
  uint8_t rgb[] = {255,0,0, 0,255,0, 0,0,255, 255,255,255, 0,0,0, 128,64,32};
  JRECT region{7,9,11,12};
  assert(paintJpeg(nullptr, rgb, &region) == 1);
  assert((rendered == std::vector<uint16_t>{0xf800,0x07e0,0x001f,0xffff,0,0x8204}));
  assert(yields == 1);
  region.right = 240;
  assert(paintJpeg(nullptr, rgb, &region) == 0);
  region.right = 23;
  assert(paintJpeg(nullptr, rgb, &region) == 0);
  assert(rendered.size() == 6);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)/'rgb.cpp'
    output = Path(directory)/'rgb'
    path.write_text(fixture + paint + checks)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(output)], check=True)
    subprocess.run([str(output)], check=True)
print('Actual JPEG callback RGB565 colors, multirow stride and bounds passed')
