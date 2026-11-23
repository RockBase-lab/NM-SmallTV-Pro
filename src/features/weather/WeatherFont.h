#pragma once
#include <cstdint>
#ifdef ARDUINO
#include <pgmspace.h>
#endif

namespace weatherfont {
inline uint8_t byte(const uint8_t* address) {
#ifdef ARDUINO
  return pgm_read_byte(address);
#else
  return *address;
#endif
}
inline void unpack(const uint8_t* source, uint8_t* output, unsigned size) {
  unsigned written=0;
  while (written<size) {
    unsigned flags=byte(source++);
    for (unsigned bit=0;bit<8 && written<size;++bit) {
      if (flags&(1<<bit)) output[written++]=byte(source++);
      else {
        unsigned token=byte(source++)<<8;token|=byte(source++);
        unsigned distance=(token>>6)+1,count=(token&63)+3;
        while (count-- && written<size) {output[written]=output[written-distance];++written;}
      }
    }
  }
}
}
