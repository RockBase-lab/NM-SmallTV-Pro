// Platform.cpp — the few things that need a real definition (not header-inline)
// but are still core-specific. Keeps the per-chip SNTP callback wiring out of the
// feature code, same as the inline shims in Platform.h.
#include "Platform.h"
#include "config.h"
#include "CrashRecord.h"

#if defined(SMALLTV_FAMILY_ESP32)
#include <esp_sntp.h>

static void (*s_syncCb)() = nullptr;
static void sntpSyncNotify(struct timeval*) { if (s_syncCb) s_syncCb(); }

void platformOnTimeSync(void (*cb)()) {
  s_syncCb = cb;
  sntp_set_time_sync_notification_cb(sntpSyncNotify);
}

#if defined(SMALLTV_ESP32) && !defined(SMALLTV_ESP32_PRO)
#include "Esp32CrashRecord.h"
#include <esp_attr.h>
#include <esp_private/panic_internal.h>

static RTC_NOINIT_ATTR Esp32CrashSnapshot s_crashSnapshot;
static DRAM_ATTR char s_crashVersion[] = FW_VERSION;
static DRAM_ATTR char s_abortReason[] = "Abort/assert";
static DRAM_ATTR char s_crashOperation[24];
static DRAM_ATTR uint32_t s_crashUptime;
static DRAM_ATTR bool s_crashClockKnown;
static bool s_currentCrash;

static void IRAM_ATTR capturePanic(arduino_panic_info_t* info, void*) {
  s_crashSnapshot.magic = 0;
  s_crashSnapshot.pc = reinterpret_cast<uintptr_t>(info->pc);
  s_crashSnapshot.cause = 0;
  s_crashSnapshot.address = 0;
  s_crashSnapshot.flags = 1 | (s_crashClockKnown ? 2 : 0);
  s_crashSnapshot.backtraceCount = info->backtrace_len < 12 ? info->backtrace_len : 12;
  for (unsigned index = 0; index < 12; ++index)
    s_crashSnapshot.backtrace[index] = index < s_crashSnapshot.backtraceCount ? info->backtrace[index] : 0;
  s_crashSnapshot.uptimeS = s_crashUptime;
  const char* reason = g_panic_abort ? (g_panic_abort_details ? g_panic_abort_details : s_abortReason) : info->reason;
  esp32CrashCopy(s_crashSnapshot.reason, sizeof(s_crashSnapshot.reason), reason ? reason : s_abortReason);
  esp32CrashCopy(s_crashSnapshot.fw, sizeof(s_crashSnapshot.fw), s_crashVersion);
  esp32CrashCopy(s_crashSnapshot.operation, sizeof(s_crashSnapshot.operation), s_crashOperation);
  crashSeal(s_crashSnapshot);
}

void platformCrashTrace(const char* operation) {
  s_crashUptime = millis() / 1000;
  s_crashClockKnown = true;
  esp32CrashCopy(s_crashOperation, sizeof(s_crashOperation), operation);
}

void platformCrashRecordBegin() {
  s_currentCrash = esp32CrashCurrent(s_crashSnapshot, platformResetInfo().wasCrash);
  if (esp_reset_reason() == ESP_RST_POWERON || !esp32CrashValid(s_crashSnapshot)) s_crashSnapshot.magic = 0;
  if (esp32CrashValid(s_crashSnapshot)) {
    s_crashSnapshot.flags &= ~1u;
    crashSeal(s_crashSnapshot);
  }
  platformCrashTrace("boot");
  set_arduino_panic_handler(capturePanic, nullptr);
}

bool platformCrashRecordGet(PlatformCrashRecord& out) {
  if (!esp32CrashValid(s_crashSnapshot)) return false;
  memset(&out, 0, sizeof(out));
  esp32CrashCopy(out.reason, sizeof(out.reason), s_crashSnapshot.reason);
  esp32CrashCopy(out.fw, sizeof(out.fw), s_crashSnapshot.fw);
  esp32CrashCopy(out.operation, sizeof(out.operation), s_crashSnapshot.operation);
  out.hasPc = s_crashSnapshot.pc != 0;
  out.epc1 = s_crashSnapshot.pc;
  out.exccause = s_crashSnapshot.cause;
  out.excvaddr = s_crashSnapshot.address;
  out.uptimeS = s_crashSnapshot.uptimeS;
  out.uptimeKnown = (s_crashSnapshot.flags & 2) != 0;
  out.current = s_currentCrash;
  out.backtraceCount = s_crashSnapshot.backtraceCount < 12 ? s_crashSnapshot.backtraceCount : 12;
  memcpy(out.backtrace, s_crashSnapshot.backtrace, sizeof(out.backtrace));
  return true;
}

void platformCrashRecordClear() { s_crashSnapshot.magic = 0; s_currentCrash = false; }
#else
void platformCrashRecordBegin() {}
bool platformCrashRecordGet(PlatformCrashRecord&) { return false; }
void platformCrashRecordClear() {}
#endif

#else  // ESP8266
#include <coredecls.h>

static void (*s_syncCb)() = nullptr;
// settimeofday_cb fires whenever the clock is set; from_sntp distinguishes an
// SNTP update (what we care about) from a manual settimeofday (which we never do).
static void sntpSyncNotify(bool from_sntp) { if (from_sntp && s_syncCb) s_syncCb(); }

void platformOnTimeSync(void (*cb)()) {
  s_syncCb = cb;
  settimeofday_cb(sntpSyncNotify);
}

// ---- crash record in RTC user memory ---------------------------------------
// The RTC block has 128 user words (512 B). The core's OTA hand-off to eboot
// (struct eboot_command at 0x60001200) takes the first 32 of them (128 bytes,
// see the note in the core's Esp.cpp): anything written there between
// Update.end() and the reboot corrupts the command and eboot boots the old
// image. The record sits at word 96, well clear of it. It is 12 words, ending
// at word 107.
//
// Written only on an abnormal reset: custom_crash_callback() covers exceptions,
// the software watchdog, abort()/out of memory and stack smashes, and is the
// only place the uptime is still known; the hardware watchdog never reaches it,
// so platformCrashRecordBegin() fills the record from rst_info at the next boot,
// with the uptime marked unknown.
// A normal restart leaves the record alone (CrashRecord.h has the decisions).
static const uint32_t CRASH_RTC_WORD = 96;
static_assert(CRASH_RST_WDT == REASON_WDT_RST && CRASH_RST_EXCEPTION == REASON_EXCEPTION_RST &&
              CRASH_RST_SOFT_WDT == REASON_SOFT_WDT_RST, "CrashReasonCode out of step with the SDK");
static_assert(sizeof(CrashRtc) % 4 == 0 && sizeof(CrashRtc) / 4 + CRASH_RTC_WORD <= 128,
              "crash record must fit the RTC user area");

static bool crashLoad(CrashRtc& r) {
  if (!ESP.rtcUserMemoryRead(CRASH_RTC_WORD, reinterpret_cast<uint32_t*>(&r), sizeof(r))) return false;
  return r.magic == CRASH_MAGIC && r.check == crashCheck(r);
}

static void crashStore(CrashRtc& r) {
  r.magic = CRASH_MAGIC;
  r.check = crashCheck(r);
  ESP.rtcUserMemoryWrite(CRASH_RTC_WORD, reinterpret_cast<uint32_t*>(&r), sizeof(r));
}

// The core calls this from its exception, software-watchdog, abort and stack-smash handlers, with
// the heap and the flash cache in whatever state the fault left them. So no
// allocation and no Arduino String in here: static buffers and plain stores.
extern "C" void custom_crash_callback(struct rst_info* ri, uint32_t, uint32_t) {
  CrashRtc r;
  memset(&r, 0, sizeof(r));
  r.flags       = CRASH_PENDING;
  r.exccause    = ri->exccause;
  r.epc1        = ri->epc1;
  r.excvaddr    = ri->excvaddr;
  r.uptimeS     = millis() / 1000;
  r.rstReason   = ri->reason;
  r.uptimeKnown = 1;
  strlcpy(r.fw, FW_VERSION, sizeof(r.fw));
  crashStore(r);
}

void platformCrashRecordBegin() {
  struct rst_info* ri = ESP.getResetInfoPtr();
  if (!ri) return;
  CrashRtc r;
  bool have = crashLoad(r);
  switch (crashBootAction(have, have && (r.flags & CRASH_PENDING), ri->reason)) {
    case CRASH_KEEP:
      return;
    case CRASH_ADOPT:   // custom_crash_callback() wrote this one
      r.flags = 0;
      crashStore(r);
      return;
    case CRASH_CAPTURE:
      // Hardware watchdog, or a crash the callback didn't see: the ROM still
      // left the registers in rst_info, only the uptime is gone.
      memset(&r, 0, sizeof(r));
      r.exccause  = ri->exccause;
      r.epc1      = ri->epc1;
      r.excvaddr  = ri->excvaddr;
      r.rstReason = ri->reason;
      strlcpy(r.fw, FW_VERSION, sizeof(r.fw));   // the booted image, not necessarily the crashed one
      crashStore(r);
      return;
  }
}

bool platformCrashRecordGet(PlatformCrashRecord& out) {
  CrashRtc r;
  if (!crashLoad(r)) return false;
  memset(&out, 0, sizeof(out));
  strlcpy(out.reason, crashReasonLabel(r.rstReason), sizeof(out.reason));
  crashCopyFw(out.fw, sizeof(out.fw), r);
  out.hasRegisters = crashHasRegisters(r.rstReason);
  out.exccause    = r.exccause;
  out.epc1        = r.epc1;
  out.excvaddr    = r.excvaddr;
  out.uptimeS     = r.uptimeS;
  out.uptimeKnown = r.uptimeKnown != 0;
  return true;
}

void platformCrashRecordClear() {
  uint32_t zero[sizeof(CrashRtc) / 4] = {0};
  ESP.rtcUserMemoryWrite(CRASH_RTC_WORD, zero, sizeof(zero));
}
#endif
