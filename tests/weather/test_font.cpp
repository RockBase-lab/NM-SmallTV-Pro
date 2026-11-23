#include "WeatherDigits.h"
#include "WeatherFont.h"
#include "WeatherSpaceman.h"
#include <cstdio>
int main(int argumentCount, char**) {
  uint8_t samples[800];
  if (argumentCount > 1) {
    for (unsigned frame=0;frame<WeatherSpacemanFrames;++frame) {
      weatherfont::unpack(weatherSpacemanPixels+weatherSpacemanOffsets[frame],samples,392);
      if (fwrite(samples,1,392,stdout)!=392) return 1;
    }
    return 0;
  }
  for (unsigned i=0;i<10;++i) {
    weatherfont::unpack(weatherDigits+weatherDigitOffsets[i],samples,800);
    if (fwrite(samples,1,800,stdout)!=800) return 1;
  }
  for (unsigned i=0;i<95;++i) {
    unsigned size=weatherLetters[i].width*24/4;
    weatherfont::unpack(weatherLetterPixels+weatherLetters[i].offset,samples,size);
    if (fwrite(samples,1,size,stdout)!=size) return 1;
  }
}
