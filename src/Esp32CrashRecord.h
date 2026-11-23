#pragma once
#include <stddef.h>
#include <stdint.h>

struct Esp32CrashSnapshot {
  uint32_t magic, check;
  uint32_t pc, cause, address, uptimeS, flags;
  char reason[48], fw[16], operation[24];
  uint32_t backtrace[12], backtraceCount;
};

static inline __attribute__((always_inline)) uint32_t esp32CrashCheck(const Esp32CrashSnapshot& snapshot) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&snapshot);
  uint32_t result = 2166136261u;
  for (size_t index = 8; index < sizeof(snapshot); ++index) {
    result ^= bytes[index];
    result *= 16777619u;
  }
  return result;
}

static inline __attribute__((always_inline)) void crashSeal(Esp32CrashSnapshot& snapshot) {
  snapshot.check = esp32CrashCheck(snapshot);
  snapshot.magic = 0x4e4d4331;
}

static inline bool esp32CrashValid(const Esp32CrashSnapshot& snapshot) {
  return snapshot.magic == 0x4e4d4331 && snapshot.check == esp32CrashCheck(snapshot);
}

static inline bool esp32CrashCurrent(const Esp32CrashSnapshot& snapshot, bool crashReset) {
  return crashReset && esp32CrashValid(snapshot) && (snapshot.flags & 1);
}

static inline __attribute__((always_inline)) void esp32CrashCopy(char* destination, size_t capacity, const char* source) {
  size_t index = 0;
  for (; index + 1 < capacity && source[index]; ++index) destination[index] = source[index];
  for (; index < capacity; ++index) destination[index] = 0;
}
