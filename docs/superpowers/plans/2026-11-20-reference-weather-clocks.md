# Reference Weather Clocks Implementation Plan

**Goal:** Deliver the four supplied 240×240 clock/weather layouts using Open-Meteo and complete the existing album management controls.

**Architecture:** Keep the existing weather mode and shared cache. Add four native layouts selected by `weatherStyle` (simple, dashboard, forecast, stacked), with optional rotation while the mode is displayed. Native drawing shares icons and large digits, consumes no extra installed-package slots, and keeps existing custom theme packages available. Open-Meteo supplies current conditions and three daily forecasts in one bounded HTTPS request; failed responses never replace cached data. Album management uses existing theme install/delete endpoints.

**Tech Stack:** Arduino C++, ArduinoJson, Arduino_GFX, existing PlatformIO targets; Node VM page tests and portable C++ parser tests.

The user's reference images replace the previous proposed analog theme. Implement in the current working tree because its existing uncommitted NM-TV-154 changes are required inputs. Do not commit or create a branch without user instruction.

## Tasks

- [x] Add portable parser tests: zero values, negative temperatures, three forecast dates/codes/min/max, sunrise/sunset, null/malformed/missing fields, transactional cache preservation. Add page tests for style persistence and album upload/list/delete operations.
- [x] Add `WeatherForecast.h` and `OpenMeteo.h` with fixed-size forecast storage and validated parsing; switch `WeatherClient.cpp` to HTTPS with HTTP/1.0, bounded body size, explicit Celsius/km/h, `timezone=auto` and three days. Preserve retry backoff and cached success on failures.
- [x] Implement shared large angular digits and vector weather icons in `WeatherClock.h/.cpp`; render simple weather, dashboard, three-day forecast, and diagonal large time. Show clock before weather arrives, update seconds once per second, show stale/error status, use requested units. Cache location changes separately from layout changes.
- [x] Persist `weatherStyle`, `weatherAuto`, `weatherRotateSec`; expose four directly selectable style buttons plus rotation controls in Weather. Default existing configs to simple. Arm NTP for weather modes, including manual touch preview. Reset rotation dwell on wake, without saving each transition.
- [x] Complete picture list, upload, deletion, refresh, grouping and bilingual feedback. Keep package-based import for this iteration, explicitly describe existing compressed animations. Exercise failed requests and invalid packages.
- [x] Run parser and Node regressions, regenerate `src/webui.h`, build `smalltv_esp32_wg` and `smalltv_esp32`, inspect OTA size and LittleFS packaging, produce layout previews from actual drawing commands, update product documentation with measured results and remaining hardware checks.

UI tests failed before their implementation; the first runnable native parser test was after implementation because the Windows host compiler was broken. The manual-entry NTP regression was observed failing on the original demand guard and passed with the fix. Independent review found that NTP issue; its scoped fix was reviewed again with no further findings. No device was flashed or runtime-tested.

## Verification Commands

`node tests/weather/test_webui.js`

`node tests/weather/test_clocks.js`

`node tests/theme/test_webui_pictures.js`

`node tests/theme/test_album_management.js`

`python tools/gzip_webui.py --check`

`pio run -e smalltv_esp32_wg -e smalltv_esp32`

Portable host tests use the available C++ compiler and `.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src`; where host tools are unavailable, use the existing WSL toolchain and report its result. Firmware builds remain mandatory. Device flashing is outside this iteration; do not claim on-device verification.
