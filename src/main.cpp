// smalltv-mod — custom firmware for the GeekMagic SmallTV (ESP-12F / ESP8266)
//
// Three features, each a self-contained DisplayMode (see Mode.h), picked in the
// web UI and dispatched from the registry below:
//   - Ticker (features/ticker):  stock/crypto price, % change, sparkline.
//   - Usage  (features/usage):   Claude 5h/7d usage bars + animated mascot.
//   - Radar  (features/radar):   live ADS-B plane radar (compiled in when WITH_RADAR).
// Shared plumbing (WiFi, web UI, OTA, display core, settings) lives at src root.
//
// License: WTFPL
#include <Arduino.h>
#include "Platform.h"
#include "config.h"
#include "Settings.h"
#include "Net.h"
#include "Gfx.h"
#include "WebPortal.h"
#include "OtaUpdate.h"
#include "Mode.h"
#include "Clock.h"
#include "SunTimes.h"
#include "WgClient.h"
#if WITH_THEME
#include "features/theme/ThemeMode.h"
#endif
#if WITH_NOTIFY
#include "NotifyMode.h"
#endif
#include "BurninWiper.h"

#if WITH_TICKER
#include "TickerMode.h"
#include "features/ticker/StockClient.h"
#endif
#include "features/weather/WeatherMode.h"
#include "features/weather/WeatherClient.h"
#if WITH_USAGE
#include "UsageMode.h"
#endif
#if WITH_RADAR
#include "RadarMode.h"
#endif
#if WITH_HA
#include "HaBoot.h"
#include "HaMode.h"
#include "MqttClient.h"
#endif

// ---- mode registry --------------------------------------------------------
// The compiled-in features, in display order. main.cpp holds no per-feature
// state of its own — each mode owns its fetch/render/dirty tracking.
static DisplayMode* kModes[] = {
#if WITH_TICKER
  &g_tickerMode,
  &g_tickerMode2,
#if WITH_CRYPTO
  &g_cryptoMode,
#endif
#endif
  &g_weatherMode,
#if WITH_USAGE
  &g_usageMode,
#endif
#if WITH_RADAR
  &g_radarMode,
#endif
#if WITH_HA
  &g_haMode,
#endif
#if WITH_THEME
  &g_themeMode,
#if WITH_PICTURES
  &g_pictureMode,
#endif
#endif
};
static const size_t kModeCount = sizeof(kModes) / sizeof(kModes[0]);

// ---- carousel -------------------------------------------------------------
// MODE_CAROUSEL rotates through the ticked features. Switches call wake() on
// the incoming mode: repaint from cached data, no refetch.
static size_t   g_carIdx = 0;
static uint32_t g_carSwitch = 0;
static int      g_manualIdx = -1;      // button preview across every compiled mode; -1 = automatic
static uint32_t g_manualUntil = 0;
static DisplayMode* g_visibleMode = nullptr;
static bool g_modeMessageDrawn = false;

static bool carouselHas(const Settings& s, const DisplayMode* m) {
  switch (m->modeConst()) {
    case MODE_STOCKS:  return s.carouselTicker;
    case MODE_STOCKS2: return s.carouselTicker2;
#if WITH_CRYPTO
    case MODE_CRYPTO:  return s.carouselCrypto;
#endif
    case MODE_WEATHER: return s.carouselWeather;
    case MODE_USAGE:   return s.carouselUsage;
    case MODE_RADAR:  return s.carouselRadar;
#if WITH_HA
    case MODE_HA:     return s.carouselHa;
#endif
#if WITH_THEME
    case MODE_THEME:  return s.carouselTheme;
#if WITH_PICTURES
    case MODE_PICTURES: return s.carouselPictures;
#endif
#endif
    default:          return false;
  }
}

// Advance g_carIdx to the next ticked mode (stays put if none other is ticked).
static void carouselNext(const Settings& s) {
  for (size_t hop = 1; hop <= kModeCount; hop++) {
    size_t cand = (g_carIdx + hop) % kModeCount;
    if (!carouselHas(s, kModes[cand])) continue;
    g_carIdx = cand;
    return;
  }
}

// Global abort flag for network operations
volatile bool g_abortFetch = false;

static uint8_t modeFromApp(const String& app) {
  if (app.equalsIgnoreCase("stocks2") || app.equalsIgnoreCase("stock2")) return MODE_STOCKS2;
  if (app.equalsIgnoreCase("stock1")) return MODE_STOCKS;
  if (app.equalsIgnoreCase("picture")) return MODE_PICTURES;
  for (size_t index = 0; index < kModeCount; ++index)
    if (app.equalsIgnoreCase(kModes[index]->id())) return kModes[index]->modeConst();
  return 255;
}


static uint8_t resolveTimerMode(const Settings& s) {
  struct tm now;
  if (!clockNow(now)) return 255;

  int curMin = now.tm_hour * 60 + now.tm_min;
  int minElapsed = 9999;
  uint8_t selectedMode = 255;
  bool found = false;

  for (uint8_t i = 0; i < 4; i++) {
    const AppTimerItem& item = s.timers.items[i];
    if (!item.enabled) continue;
    const uint8_t app = modeFromApp(item.app);
    if (app == 255 || item.time.length() != 5 || item.time[2] != ':') continue;
    bool digits = true;
    for (uint8_t index : {0, 1, 3, 4}) if (!isdigit((unsigned char)item.time[index])) digits = false;
    if (!digits) continue;
    const int hour = (item.time[0] - '0') * 10 + item.time[1] - '0';
    const int minute = (item.time[3] - '0') * 10 + item.time[4] - '0';
    if (hour > 23 || minute > 59) continue;
    int tMin = hour * 60 + minute;

    int elapsed = (curMin >= tMin) ? (curMin - tMin) : (curMin + 1440 - tMin);
    if (elapsed < minElapsed) {
      minElapsed = elapsed;
      selectedMode = app;
      found = true;
    }
  }

  if (!found) return 255;
  return selectedMode;
}


static DisplayMode* activeMode(const Settings& s) {
  if (g_manualIdx >= 0 && (size_t)g_manualIdx < kModeCount) {
    if (s.mode != MODE_TIMER || (int32_t)(millis() - g_manualUntil) < 0) return kModes[g_manualIdx];
    g_manualIdx = -1;
  }
  if (s.mode == MODE_CAROUSEL && kModeCount > 0) {
    if (g_carSwitch == 0) g_carSwitch = millis();
    if (!carouselHas(s, kModes[g_carIdx])) {
      carouselNext(s);
      g_carSwitch = millis();
    }
    if (!carouselHas(s, kModes[g_carIdx])) return nullptr;
    bool dwellExpired = millis() - g_carSwitch >= (uint32_t)s.carouselSec * 1000UL;
    if (dwellExpired) {
      g_carSwitch = millis();
      carouselNext(s);
    }
    return kModes[g_carIdx];
  }
  if (s.mode == MODE_TIMER && kModeCount > 0) {
    uint8_t target = resolveTimerMode(s);
    for (size_t i = 0; i < kModeCount; i++) {
      if (kModes[i]->modeConst() == target) return kModes[i];
    }
    return nullptr;
  }
  for (size_t i = 0; i < kModeCount; i++)
    if (kModes[i]->modeConst() == s.mode) return kModes[i];
  return nullptr;
}

static Settings g_settings;
static String   g_resetReason;        // why the chip last reset (diagnostics)
static bool     g_safeMode = false;   // last reset was an exception -> don't re-enter the crash
static char     g_epcStr[16] = "";
static char     g_addrStr[16] = "";
static int g_lastBr = -1;        // last effective brightness written (-1 = none yet)
static uint32_t g_btnWakeUntil = 0;  // button wake overrides dimming until this millis()
#if HAS_LDR
static uint32_t g_lastAutoBr = 0;
static uint8_t  g_ldrCache   = DEFAULT_BRIGHTNESS;   // last LDR reading (2 s cadence)
#endif

// Single brightness resolver: night mode overrides auto-brightness overrides the
// manual level. Only writes the PWM when the effective target changes.
// The order is in resolveBrightness() (SunTimes.cpp), which the native tests
// cover. The result is in backlight steps (0..255) so the sun fade can move as
// finely as the PWM allows.
static uint8_t appEffectiveBrightness() {
  BrightnessInputs in = {};
  // A button wake press briefly lifts the screen to the manual level so a touch
  // reveals it even when night mode has dimmed or blanked the backlight.
  in.wake = g_btnWakeUntil && millis() < g_btnWakeUntil;
  in.sliderPct = g_settings.brightness;
  in.nightActive = clockNightActive();
  in.nightPct = g_settings.clock.nightLevel;
  in.overlap = g_settings.clock.sunOverlap;
  in.sunValid = clockSunSteps(g_settings, brightnessSteps(g_settings.brightness), in.sunSteps);
#if HAS_LDR
  if (g_settings.autoBrightness) {
    if (millis() - g_lastAutoBr > 2000) {
      g_lastAutoBr = millis();
      int raw = analogRead(LDR_PIN);
      g_ldrCache = (uint8_t)constrain(raw * 100 / ADC_MAX, 5, 100);
    }
    in.ldrOn = true;
    in.ldrPct = g_ldrCache;
  }
#endif
  return resolveBrightness(in);
}

int appBacklightSteps() { return g_lastBr; }   // last level written, 0..255 (-1: none yet)

void appApplyBrightness() {
  uint8_t t = appEffectiveBrightness();
  if ((int)t != g_lastBr) {
    g_lastBr = t;
    gfxSetBrightnessSteps(t, g_settings.backlightInverted);
  }
}

#ifdef BTN_PIN
// Capacitive touch button (e.g. NM-TV-154, GPIO32). 
// touchRead() returns a capacitance count that drops when a finger is present; the press
// threshold is calibrated from an idle baseline (recomputed if the configured
// sensitivity changes), since counts vary per unit. A short and a long press
// each run a configurable action (Settings.button). Non-blocking except for the
// deliberate ~1 s screen-off blink.
static uint32_t g_btnInfoUntil = 0;   // network-info screen shown until this millis()

static void buttonBlinkOff() {
  gfxSetBrightness(0, g_settings.backlightInverted);   // blank the panel
  delay(1000);
  g_lastBr = -1;                       // force the next tick to restore brightness
  appApplyBrightness();
}

static void buttonShowNetInfo() {
  gfxStaInfo(netSSID().c_str(), netIP().c_str(), g_settings.hostname.c_str());
  g_btnInfoUntil = millis() + BTN_INFO_MS;
}

static void buttonWake() {
  g_btnWakeUntil = millis() + BTN_WAKE_MS;  // the brightness resolver lifts dimming for this window
  g_lastBr = -1;                            // force the next tick to apply it
  appApplyBrightness();
}

static void buttonAckNotify() {
#if WITH_NOTIFY
  g_notifyMode.dismiss();   // expire the active overlay now; the next one (or the feature) takes over
#endif
}

static void buttonGoHome() {
  g_manualIdx = -1;                    // leave touch preview
  g_visibleMode = nullptr;
  g_modeMessageDrawn = false;
  if (g_settings.mode == MODE_CAROUSEL && kModeCount > 0) {
    g_carIdx = 0;
    g_carSwitch = millis();
    if (!carouselHas(g_settings, kModes[0])) carouselNext(g_settings);  // 0 not ticked -> first ticked
  }
}

static void buttonNext() {
  if (kModeCount == 0) return;
  if (g_settings.mode == MODE_CAROUSEL) {
    g_manualIdx = -1;
    carouselNext(g_settings);
    g_carSwitch = millis();
    return;
  }

  // Resolve the current page once, then walk the complete compiled registry.
  // Do not rewrite settings.mode or the Timer resolver's state for a preview.
  DisplayMode* current = activeMode(g_settings);
  int curIdx = -1;
  for (size_t i = 0; i < kModeCount; i++) {
    if (kModes[i] == current) { curIdx = (int)i; break; }
  }
  g_manualIdx = (curIdx >= 0) ? ((curIdx + 1) % kModeCount) : 0;
  g_manualUntil = millis() + 15000;
  g_visibleMode = nullptr;

  // Rapid touches repaint cached pages immediately; network fetches follow shortly.
  for (uint8_t set = 0; set < NUM_STOCK_SETS; set++) stocksDeferNextFetch(set, 800);
  weatherDeferNextFetch(800);
  g_abortFetch = false;
}

static void buttonAction(uint8_t act) {
  g_abortFetch = true; // Signal any in-flight network fetch to abort immediately
  switch (act) {
    case BTN_ACT_SCREEN_OFF: buttonBlinkOff();    break;
    case BTN_ACT_HOME:       buttonGoHome();      break;
    case BTN_ACT_NEXT:       buttonNext();        break;
    case BTN_ACT_NETINFO:    buttonShowNetInfo(); break;
    case BTN_ACT_WAKE:       buttonWake();        break;
    case BTN_ACT_ACK:        buttonAckNotify();   break;
    default: break;                                 // BTN_ACT_NONE
  }
  g_abortFetch = false;
}

static void buttonService() {
  static bool     inited   = false;
  static uint16_t curBaseline = 0;
  static uint16_t thrDown  = 0;       // press  when median < thrDown
  static uint16_t thrUp    = 0;       // release when median > thrUp (hysteresis band)
  static uint8_t  sensSeen = 0;       // sensitivity the thresholds were computed for
  static bool     stable   = false;   // confirmed (debounced) button state
  static uint8_t  run      = 0;       // consecutive samples disagreeing with 'stable'
  static uint32_t downAt   = 0;
  static uint32_t lastBaseUpdate = 0;

  // Calibrate (or recalibrate when the web UI changes the sensitivity).
  if (!inited || sensSeen != g_settings.button.sensitivity) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) { sum += touchRead(BTN_PIN); delay(2); }
    curBaseline = sum / 16;
    uint8_t s = g_settings.button.sensitivity;
    thrDown = (uint16_t)((uint32_t)curBaseline * s / 100);
    uint8_t up = s + 5; if (up > 99) up = 99;            // ~5 % hysteresis band
    thrUp = (uint16_t)((uint32_t)curBaseline * up / 100);
    sensSeen = s;
    inited = true;
    lastBaseUpdate = millis();
  }

  // ESP32 touchRead() emits isolated low-going glitch samples; a median of three
  // reads rejects any single spike, so a lone glitch can no longer be seen as a
  // press. Hysteresis then keeps the state from chattering near the threshold.
  uint16_t a = touchRead(BTN_PIN);
  uint16_t b = touchRead(BTN_PIN);
  uint16_t c = touchRead(BTN_PIN);
  uint16_t m = a > b ? (b > c ? b : (a > c ? c : a))
                     : (a > c ? a : (b > c ? c : b));

  // Slow EMA tracking of idle baseline to adapt to temperature / humidity / power supply drift
  if (!stable && m > thrUp && (millis() - lastBaseUpdate > 500)) {
    lastBaseUpdate = millis();
    // Smoothly adjust baseline towards higher idle values
    if (m > curBaseline || curBaseline - m < 15) {
      curBaseline = (curBaseline * 7 + m) / 8;
      uint8_t s = g_settings.button.sensitivity;
      thrDown = (uint16_t)((uint32_t)curBaseline * s / 100);
      uint8_t up = s + 5; if (up > 99) up = 99;
      thrUp = (uint16_t)((uint32_t)curBaseline * up / 100);
    }
  }

  bool wantDown = stable ? (m < thrUp) : (m < thrDown);
  if (wantDown == stable) { run = 0; return; }

  // Integrator debounce: require 2 consecutive agreeing samples before
  // flipping the confirmed state, so response is fast and responsive.
  if (++run < 2) return;
  run = 0;
  stable = wantDown;
  if (stable) {
    downAt = millis();
    g_abortFetch = true; // Signal in-flight fetch to abort immediately on touch contact
    if (burninActive()) {
      burninDismiss();
      return;
    }
    return;
  }
  uint32_t held = millis() - downAt;
  buttonAction(held >= BTN_LONG_MS ? g_settings.button.longAction
                                   : g_settings.button.shortAction);
}
#endif

void buttonPoll() {
#ifdef BTN_PIN
  buttonService();
#endif
}

// Exposed to the web portal (/api/status) so the last reset reason is visible.
const char* appResetReason() { return g_resetReason.c_str(); }
bool appSafeMode() { return g_safeMode; }

// Called by the web portal after settings are applied: re-init every mode and
// force a fresh repaint so a mode/URL/symbol change takes effect immediately.
void appInvalidate() {
  g_manualIdx = -1;
  g_carIdx = 0;
  g_carSwitch = millis();
  g_visibleMode = nullptr;
  g_modeMessageDrawn = false;
  for (size_t i = 0; i < kModeCount; i++) kModes[i]->invalidate(g_settings);
}

// Called by the web portal before a request that needs a lot of heap, such as
// a settings save: every mode frees what it can rebuild on its next service().
void appRelease() {
  for (size_t i = 0; i < kModeCount; i++) kModes[i]->freeForRequest();
}

static void bootProgress(const char* msg) {
  gfxBoot(FW_DISPLAY_NAME, msg);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(FW_NAME " " FW_VERSION);

  // Capture why we (re)booted. On a reboot loop this is the key clue, and the
  // device's UART isn't exposed — so we also show it on screen below. On the
  // ESP8266 we also keep the crash PC (epc1) for addr2line decoding; the
  // ESP32-C2 (RISC-V) doesn't expose it, so epc/addr come back empty there.
  PlatformReset pr = platformResetInfo();
  platformCrashRecordBegin();   // keep the registers/uptime of an abnormal reset in RTC memory
  Serial.print("[boot] reset reason: ");
  Serial.println(pr.reason);

  if (pr.wasCrash) {
    g_safeMode = true;                   // crashed last boot -> stay out of the crash path
    strlcpy(g_epcStr,  pr.epc,  sizeof(g_epcStr));
    strlcpy(g_addrStr, pr.addr, sizeof(g_addrStr));
    PlatformCrashRecord crash;
    if (platformCrashRecordGet(crash) && crash.current && (crash.hasRegisters || crash.hasPc)) {
      snprintf(g_epcStr, sizeof(g_epcStr), "0x%08x", (unsigned)crash.epc1);
      if (crash.hasRegisters) snprintf(g_addrStr, sizeof(g_addrStr), "0x%08x", (unsigned)crash.excvaddr);
    }
    char rich[80];
    snprintf(rich, sizeof(rich), "%s epc %s addr %s", pr.reason.c_str(),
             g_epcStr[0] ? g_epcStr : "-", g_addrStr[0] ? g_addrStr : "-");
    g_resetReason = rich;
  } else {
    g_resetReason = pr.reason;
  }

  Serial.println("[boot] settings");
  settingsBegin();
  loadSettings(g_settings);

#if WITH_HA
  // Home Assistant's memory (the screen store and the MQTT client) is taken
  // here, before the display, the Wi-Fi and the web server fragment the heap,
  // and only when a broker host is set. If the heap says no, or this is safe
  // mode, Home Assistant stays off and /api/status says why.
  haBoot(haWanted(g_settings), g_safeMode);
#endif

  Serial.println("[boot] display");
  gfxBegin(g_settings);
  gfxBoot(g_safeMode ? "Crashed" : FW_DISPLAY_NAME, FW_VERSION);

  Serial.println("[boot] net");
  netAllowRetry(!g_safeMode);   // a retry's reboot would clear the crash flag
  netBegin(g_settings, bootProgress);
  // Arm SNTP now that WiFi (STA) is up — but only if night mode is enabled, so a
  // ticker-only device doesn't pay the SNTP heap cost (which can starve the cash.ch
  // TLS handshake on the ESP8266). clockReapply arms it iff needed. Skipped after a
  // crash so a fault in here can't boot-loop before the web server starts (the
  // device then comes up in safe mode, OTA-recoverable, instead of needing UART).
  // ...unless a WireGuard tunnel is configured, which needs the clock and only
  // exists on an ESP32 where the heap argument for the skip does not apply.
  if (!g_safeMode || wgNeedsClock(g_settings) || g_settings.mode == MODE_TIMER) clockReapply(g_settings);

  // Optional WireGuard tunnel (ESP32 targets). Arms the state machine only;
  // the bring-up itself runs from loop(), so nothing here can delay the web
  // server. A crash last boot feeds the three-strikes hold that keeps a bad
  // tunnel config from locking the device out of its own web UI.
  wgBegin(g_settings, g_safeMode);

  // A GitHub update queued from the web UI runs now, before the features claim
  // the heap (the download needs a 16 KB TLS buffer that only fits at boot).
  // On success it reboots into the new image; a no-op stub on the ESP32 targets.
  if (otaBootRequested()) {
    Serial.println("[boot] github update");
    gfxBoot(FW_DISPLAY_NAME, "updating...");
    otaBootUpdate(g_settings);
    gfxBoot(FW_DISPLAY_NAME, "update failed");   // still here -> failed; details in the web UI
    delay(1200);
  }

  Serial.println("[boot] web");
  webPortalBegin(g_settings);

#if WITH_HA
  mqttBegin(g_settings);   // arms the client; the connect itself runs from loop()
#endif

  burninInit();
  Serial.println("[boot] modes");
  for (size_t i = 0; i < kModeCount; i++) kModes[i]->begin(g_settings);
  Serial.println("[boot] done");

  if (netMode() == NET_AP) {
    gfxApInfo(g_settings.apSsid.c_str(), g_settings.apPass.c_str(), netIP().c_str(), netFailReason());
  } else if (g_safeMode) {
    // Last boot crashed: show the crash address (persistent) and keep the web
    // server up for OTA recovery — don't enter the render path that crashed.
    gfxCrash(g_epcStr, g_addrStr, netIP().c_str());
  } else {
    // Show which network we joined and how to reach the web UI, long enough to read.
    gfxStaInfo(netSSID().c_str(), netIP().c_str(), g_settings.hostname.c_str());
    delay(3500);
  }
}

void loop() {
  platformCrashTrace("network");
  netLoop();
  platformCrashTrace("web");
  webPortalLoop();

#ifdef BTN_PIN
  buttonService();
  // The network-info screen (a long-press action) holds for BTN_INFO_MS, then
  // the current mode repaints over it. Handled here so it works in AP mode too.
  if (g_btnInfoUntil && millis() >= g_btnInfoUntil) {
    g_btnInfoUntil = 0;
    if (netMode() == NET_AP) {
      gfxApInfo(g_settings.apSsid.c_str(), g_settings.apPass.c_str(), netIP().c_str(), netFailReason());
    } else {
      g_visibleMode = nullptr;
      g_modeMessageDrawn = false;
    }
  }
#endif

  if (webPortalRebootDue()) {
    delay(120);
    ESP.restart();
  }

  // Before the safe-mode return on purpose: if the crash had nothing to do with
  // the tunnel, remote access survives it, and if it did, the three-strikes hold
  // stops the retries by itself.
  platformCrashTrace("wireguard");
  wgService(g_settings);

  if (g_safeMode) {
    delay(5);
    return;  // crashed last boot: web UI stays up for OTA recovery, no rendering
  }

#if WITH_HA
  // After the safe-mode return on purpose: a fault in here (e.g. a malformed
  // retained screen that re-arrives on every connect) must not boot-loop the
  // device past its own recovery page. Self-gates on WiFi STA + broker config.
  platformCrashTrace("mqtt");
  mqttLoop();
#endif

  if (netMode() == NET_AP) {
    // Setup mode: AP info stays on screen, redrawn when a background retry
    // of the saved networks fails with a new reason.
    static uint16_t shownFail = netFailSeq();
    if (netFailSeq() != shownFail) {
      shownFail = netFailSeq();
      gfxApInfo(g_settings.apSsid.c_str(), g_settings.apPass.c_str(), netIP().c_str(), netFailReason());
    }
    delay(5);
    return;
  }

#ifdef BTN_PIN
  if (g_btnInfoUntil) {
    delay(5);
    return;  // network-info screen is up; don't let the mode repaint over it
  }
#endif

  // --- STA mode: the active feature fetches + renders itself ---

  // Night-mode state machine (NTP-trust gate), then apply the effective brightness
  // (night override / auto-brightness / manual level).
  platformCrashTrace("clock");
  clockService(g_settings);
  appApplyBrightness();

  // On expiry the carousel dwell is credited back the time it was hidden, so it
  // resumes on the same feature with the same remaining slice. heldMs() spans
  // the whole run rather than the last request in it, so a queue that chained
  // four overlays credits back all four.
#if WITH_NOTIFY
  static bool wasNotifying = false;
  if (g_notifyMode.active()) {
    wasNotifying = true;
    g_notifyMode.service(g_settings);
    delay(5);
    return;
  }
  bool restore = wasNotifying;
  if (wasNotifying) {
    wasNotifying = false;
    if (g_carSwitch) g_carSwitch += g_notifyMode.heldMs();
  }
#else
  bool restore = false;
#endif

  // Screen burn-in prevention
  static bool wasBurnin = false;
  burninService(g_settings);
  if (burninActive()) {
    wasBurnin = true;
    delay(5);
    return;
  }
  if (wasBurnin) {
    wasBurnin = false;
    if (g_carSwitch) g_carSwitch += burninHeldMs();
    restore = true;
  }

  DisplayMode* m = activeMode(g_settings);
  if (m != g_visibleMode || restore) {
    g_visibleMode = m;
    g_modeMessageDrawn = false;
    if (m) { platformCrashTrace(m->id()); m->wake(g_settings); }
  }
  if (m) {
    platformCrashTrace(m->id());
    m->service(g_settings);
  } else if (!g_modeMessageDrawn) {
    gfxMessage(g_settings.mode == MODE_TIMER ? "TIMER" : "MODE",
               g_settings.mode == MODE_TIMER && !clockSynced() ? "Waiting for clock" : "Select apps in General", C_WHITE);
    g_modeMessageDrawn = true;
  }

  // Background polling for inactive features so switching apps is instantaneous:
  // stocksService self-times every ticker's schedule (shared across Stock 1 & Stock 2)
  platformCrashTrace("stock-refresh");
  stocksService(g_settings);

  delay(5);
}
