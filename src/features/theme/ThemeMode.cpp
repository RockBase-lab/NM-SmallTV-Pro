#include "ThemeMode.h"
#if WITH_THEME
#include "Clock.h"
#include "Gfx.h"
#include "Platform.h"
#include "ThemeDataClient.h"
#include "JpegPicture.h"
#include <Arduino_GFX_Library.h>
#include <font/glcdfont.h>
#include <algorithm>

ThemeMode g_themeMode;
#if WITH_PICTURES
ThemeMode g_pictureMode(true);
#endif
namespace {
// Adapts the portable engine's Display interface to this device's actual
// panel: glyphColumn() reads the built-in font from flash (PROGMEM), and
// row() applies the usual panel color tint before handing the row to the
// GFX driver. yield() every 16 rows keeps a large dirty region from starving
// WiFi/other tasks during a long render.
class Panel : public smalltv::Display {
 public:
  uint8_t glyphColumn(uint8_t c, uint8_t col) const override {
    return pgm_read_byte(font + unsigned(c) * 5 + col);
  }
  void row(int x, int y, const uint16_t* pixels, int count) override {
    uint16_t tinted[240];
    for (int i = 0; i < count; ++i) tinted[i] = gfxTint(pixels[i]);
    gfxDev()->draw16bitRGBBitmap(x, y, tinted, count, 1);
    if ((y & 15) == 15) yield();
  }
};
// What a load needs before it starts: a package parse (ThemeMode.h). A failed
// allocation mid-load restarts the ESP8266, so a load that would not fit
// waits instead.
constexpr uint32_t LoadFreeHeap = ThemeMode::ParseFreeHeap;
constexpr uint32_t LoadLargestBlock = ThemeMode::ParseLargestBlock;
const char* const WaitingForMemory = "Waiting for free memory";
}
void ThemeMode::begin(const Settings&) {
  LittleFS.mkdir("/themes");
  // An interrupted install leaves its staging file behind; boot is the one
  // point it is safe to remove without racing a request in progress.
  LittleFS.remove("/themes/.upload");
}
void ThemeMode::wake(const Settings& settings) {
  if (!pictures_) clockReapply(settings, true);
  engine_.invalidate();
  messageDrawn_ = false;
}
// Drops every bit of state for the currently loaded theme -- the open
// package/file, any in-flight or fetched data, and the engine itself -- so
// the next service() call starts a clean load from scratch.
void ThemeMode::unload() {
  dataRequests_.cancel();
  dataRequests_.take();
  package_.reset();
  file_.reset();
  dataNextMs_.clear();
  dataScheduled_.clear();
  dataValues_.clear();
  dataAges_.reset(0);
  dataCursor_ = 0;
  engine_.setValues({});
  engine_.setTheme(smalltv::Theme{}, millis());
  loadedId_ = "";
  attempted_ = messageDrawn_ = keepData_ = false;
  error_ = "";
}
void ThemeMode::release(uint32_t needFree, uint32_t needLargestBlock) {
  if (!package_ || (ESP.getFreeHeap() >= needFree && platformMaxFreeBlock() >= needLargestBlock)) return;
  // A fetch in flight is dropped; its source is still due, so the reload
  // fetches it again.
  dataRequests_.cancel();
  dataRequests_.take();
  package_.reset();
  file_.reset();
  engine_.setValues({});
  engine_.setTheme(smalltv::Theme{}, millis());
  attempted_ = messageDrawn_ = false;
  keepData_ = true;
}
// Collects a finished fetch (if any) and, with at most one request in flight
// at a time, starts the next source whose interval has elapsed, round-robin
// over sources so no single slow/failing one starves the others.
void ThemeMode::refreshData() {
  const auto& sources = engine_.theme().data;
  // The "<source>.age" values (DataAges): move the ages on, and a fetch that
  // just ended sets or leaves its source's age. millis() only; no wall clock.
  dataAges_.tick(millis());
  bool changed = false;
  if (auto result = dataRequests_.take()) {
    // A success merges its values into the running set and sets the age to 0.
    changed = smalltv::collectFetch(*result, dataAges_, dataValues_);
    // Schedule from completion, preserving the interval even for slow sources.
    dataNextMs_[result->index] = millis() + sources[result->index].interval * 1000UL;
  }
  // The ages change once a minute. The engine gets the values only when they
  // changed: a fetch's new values, or a new age.
  if (dataAges_.publish(sources, dataValues_)) changed = true;
  if (changed) engine_.setValues(dataValues_);
  if (sources.empty() || dataRequests_.busy() || WiFi.status() != WL_CONNECTED) return;
  // Scan forward from `dataCursor_` for the first source that is due (an
  // unscheduled one always is), start it, and advance the cursor past it --
  // so a source that just fired goes to the back of the round-robin queue.
  for (size_t offset = 0; offset < sources.size(); ++offset) {
    size_t i = (dataCursor_ + offset) % sources.size();
    if (dataScheduled_[i] && static_cast<int32_t>(millis() - dataNextMs_[i]) < 0) continue;
    auto request = dataRequests_.start(sources[i], i);
    if (!startThemeDataFetch(request)) request->finish(false);
    dataScheduled_[i] = true;
    dataCursor_ = (i + 1) % sources.size();
    break;
  }
}
String ThemeMode::selectedId(const Settings& s) {
  return pictures_ ? pictureRuntimeId_ : s.themeId;
}
// The screensaver rotates only picture packages, never the two clock faces.
// The runtime selection is deliberately not persisted on every transition.
String ThemeMode::nextPicture(const String& current) const {
#if WITH_PICTURES
  std::vector<String> ids;
  File dir = LittleFS.open("/themes", "r");
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    String name = entry.name();
    if (entry.isDirectory() || !name.endsWith(".stheme")) continue;
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    String id = name.substring(0, name.length() - 7);
    if (id == "pixel-motion" || id.startsWith("ultra-") || id.startsWith("picture-")) ids.push_back(id);
  }
  dir.close();
  std::sort(ids.begin(), ids.end(), [](const String& a, const String& b) { return a < b; });
  if (ids.empty()) return current;
  for (size_t i = 0; i < ids.size(); ++i) if (ids[i] == current) return ids[(i + 1) % ids.size()];
  return ids[0];
#else
  return current;
#endif
}
// Called when the mode should be redrawn from scratch, e.g. when a
// notification overlay just finished. A web settings save also returns the
// screensaver to its configured starting picture and restarts its dwell.
void ThemeMode::invalidate(const Settings& s) {
  if (pictures_) {
    pictureRuntimeId_ = s.pictureId;
    pictureNextMs_ = millis() + uint32_t(s.pictureRotateSec) * 1000UL;
  }
  const bool onScreen = pictures_
      ? (s.mode == MODE_PICTURES || (s.mode == MODE_CAROUSEL && s.carouselPictures))
      : (s.mode == MODE_THEME || (s.mode == MODE_CAROUSEL && s.carouselTheme));
  const String wanted = selectedId(s);
  if (loadedId_ != wanted || !onScreen) unload();
  // A failed load may be retried after installation or settings changes.
  if (!package_) attempted_ = false;
  engine_.invalidate();
  messageDrawn_ = false;
}
// Per-frame entry point: loads the selected package at most once per selection
// (via `attempted_`, so a failed load is not retried every call). In Pictures
// mode an expired dwell selects the next package even after such a failure.
void ThemeMode::service(const Settings& s) {
  if (pictures_) {
    if (!pictureRuntimeId_.length()) pictureRuntimeId_ = s.pictureId;
    if (!pictureNextMs_) pictureNextMs_ = millis() + uint32_t(s.pictureRotateSec) * 1000UL;
    if (s.pictureAuto && static_cast<int32_t>(millis() - pictureNextMs_) >= 0) {
      pictureRuntimeId_ = nextPicture(pictureRuntimeId_);
      pictureNextMs_ = millis() + uint32_t(s.pictureRotateSec) * 1000UL;
    }
  }
  const String wanted = selectedId(s);
  if (loadedId_ != wanted) {
    unload();
    loadedId_ = wanted;
  }
  // A web request, or the buffers of its reply, can briefly hold the heap a
  // load needs; the load then waits, checked again every frame.
  const bool loadFits = ESP.getFreeHeap() >= LoadFreeHeap && platformMaxFreeBlock() >= LoadLargestBlock;
  if (!attempted_ && !loadFits && error_ != WaitingForMemory) {
    error_ = WaitingForMemory;
    messageDrawn_ = false;
  }
  if (!attempted_ && loadFits) {
    attempted_ = true;
    std::string error;
    smalltv::Theme theme;
    if (!smalltv::validId(wanted.c_str())) error = pictures_ ? "Select an installed picture" : "Select an installed theme";
    else {
      file_.reset(new ThemeFile);
      if (!file_->open("/themes/" + wanted + ".stheme")) error = "Theme file not found";
      else {
        package_.reset(new smalltv::Package(*file_));
        if (!package_->load(theme, error)) package_.reset();
        else if (theme.id != wanted.c_str()) {
          error = "Theme ID mismatch";
          package_.reset();
        }
      }
    }
    if (!error.empty()) {
      error_ = pictures_ && error.find(".loop") != std::string::npos ? "Update picture library" : error.c_str();
      package_.reset();
      file_.reset();
    } else {
      // After a release the same package loads again, so its sources match.
      if (!keepData_ || dataNextMs_.size() != theme.data.size()) {
        dataNextMs_.assign(theme.data.size(), 0);
        dataScheduled_.assign(theme.data.size(), false);
        dataValues_.clear();
        // Only a spec 2 theme can use "<source>.age"; a spec 1 theme gets none.
        dataAges_.reset(theme.data.size(), theme.spec >= 2);
        dataCursor_ = 0;
      }
      engine_.setTheme(std::move(theme), millis());
      engine_.setValues(dataValues_);
      error_ = "";
    }
  }
  if (!package_) {
    if (!messageDrawn_) {
      gfxMessage(pictures_ ? "PICTURES" : "THEMES", error_.c_str(), C_WHITE);
      messageDrawn_ = true;
    }
    return;
  }
  refreshData();
  tm t{};
  bool valid = !pictures_ && clockNow(t);
  Panel panel;
  engine_.setLanguage(smalltv::languageFromCode(s.clock.lang.c_str()));
  auto dirty = engine_.update(millis(), valid ? &t : nullptr);
  // A render failure (a corrupt or truncated asset read mid-frame) is treated
  // like a load failure: drop the package and show an error rather than
  // risk repeatedly failing on every subsequent frame.
  uint32_t jpegOffset = 0, jpegLength = 0;
  const auto& layers = engine_.theme().layers;
  const bool jpeg = layers.size() == 1 && package_->jpeg(package_->resolve(
      smalltv::assetPath(layers[0], engine_.states()[0].frame)), jpegOffset, jpegLength);
  const bool rendered = jpeg ? (dirty.empty() || drawJpegPicture(*file_, jpegOffset, jpegLength))
                             : smalltv::render(engine_, dirty, *package_, panel);
  if (!rendered) {
    dataRequests_.cancel();
    dataRequests_.take();
    error_ = "Could not read theme asset";
    package_.reset();
    file_.reset();
    messageDrawn_ = false;
  }
}
#endif
