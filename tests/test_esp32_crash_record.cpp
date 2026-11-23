#include "Esp32CrashRecord.h"
#include <cassert>
#include <cstring>
#include <cstdio>

int main() {
  Esp32CrashSnapshot snapshot{};
  assert(!esp32CrashValid(snapshot));
  snapshot.pc = 0x40012345;
  snapshot.cause = 28;
  snapshot.address = 0;
  snapshot.uptimeS = 7200;
  snapshot.flags = 7;
  snapshot.backtraceCount = 2;
  snapshot.backtrace[0] = 0x40012345;
  snapshot.backtrace[1] = 0x40098765;
  esp32CrashCopy(snapshot.reason, sizeof(snapshot.reason), "LoadProhibited");
  esp32CrashCopy(snapshot.fw, sizeof(snapshot.fw), "3.13.1");
  esp32CrashCopy(snapshot.operation, sizeof(snapshot.operation), "stocks");
  crashSeal(snapshot);
  assert(esp32CrashValid(snapshot));
  Esp32CrashSnapshot retained = snapshot;
  assert(esp32CrashValid(retained));
  assert(esp32CrashCurrent(retained, true));
  assert(!esp32CrashCurrent(retained, false));
  retained.flags &= ~1u;
  crashSeal(retained);
  assert(!esp32CrashCurrent(retained, true));
  assert(retained.pc == 0x40012345 && retained.uptimeS == 7200);
  assert(retained.backtraceCount == 2 && retained.backtrace[1] == 0x40098765);
  assert(!std::strcmp(retained.operation, "stocks"));
  for (size_t index = 0; index < sizeof(snapshot); ++index) {
    Esp32CrashSnapshot corrupt = snapshot;
    reinterpret_cast<uint8_t*>(&corrupt)[index] ^= 1;
    assert(!esp32CrashValid(corrupt));
  }
  char bounded[5];
  esp32CrashCopy(bounded, sizeof(bounded), "longer than the destination");
  assert(!std::strcmp(bounded, "long"));
  esp32CrashCopy(bounded, sizeof(bounded), "x");
  assert(bounded[1] == 0 && bounded[4] == 0);
  retained.magic = 0;
  assert(!esp32CrashValid(retained));
  std::puts("ESP32 crash checksum, retained registers, bounded strings and clear passed");
}
