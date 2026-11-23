---
title: Building from source
description: Build any of the four board targets with PlatformIO, the lean ESP8266 and WireGuard ESP32 variants, and the ESP32 toolchain notes.
---

The four board targets share one codebase and build from [PlatformIO](https://platformio.org/). Pick the env that matches your board.

```bash
pio run -e smalltv                 # ESP8266
pio run -e smalltv_lean            # ESP8266, without HA screens, the usage meter or theme clocks
pio run -e smalltv_c2              # ESP32-C2
pio run -e smalltv_esp32           # NM-TV-154 (classic ESP32)
pio run -e smalltv_esp32_wg        # the same board, with the WireGuard client
pio run -e smalltv_esp32_8mb       # SmallTV Pro (classic ESP32, 8 MB flash)
pio run -e smalltv_c2 -t upload    # build + flash the C2 over USB-C
pio device monitor -e smalltv_c2   # serial logs @ 115200
pio run -e smalltv_loader          # ESP8266 loader for the SmallTV-ultra
```

Seven firmware envs cover the four boards. This fork's default CI build publishes the SmallTV Pro and full-feature NM-TV-154 WireGuard images plus the loader; the other envs remain available for direct builds. The NM-TV-154 pair shares the same dedicated layout and full display/data feature set, with WireGuard as the only difference. Asset names are listed in [Which release file to download](/NM-SmallTV-Pro/reference/release-assets/).

## Build the configuration page

NMMiner SmallTV Pro bundles `src/webui-search.js` into `src/webui.html` and minifies generated JavaScript with a locked Terser version. Source files remain readable. Install build tools with `npm ci --prefix tools`, then run `python tools/gzip_webui.py` after editing either source. Use `--small` with Python's `zopfli` package for the tight NM-TV-154 OTA build. `python tools/gzip_webui.py --check` compares the decompressed header with a fresh canonical bundle. Both sources, the pinned tools and the regenerated header belong in the change.

Weather font coverage is stored losslessly in `WeatherDigits.h`; `tools/build_weather_font.py` requires Pillow to regenerate it. The native decoder regression compares all 105 glyphs against the source raster samples. These optimizations retain existing OTA partitions and device features.

## The smalltv_lean env

`smalltv_lean` builds the same ESP8266 code as `smalltv` with Home Assistant screens, the Claude usage meter and theme clocks compiled out. It exists to buy heap, and it is published as `nm-smalltv-pro-firmware-lean.bin`.

```bash
pio run -e smalltv_lean
```

The ESP8266 shares one 80 KB DRAM arena between static allocations and the heap, so a byte of static dropped is a byte the heap gains. On the 3.5.0 build:

| Build | Static RAM | Heap gained |
|-------|-----------|-------------|
| `smalltv` | 56,984 B | baseline |
| `smalltv_lean` | 48,820 B | 8,164 B |

Lean compiles out Home Assistant, the usage meter and theme clocks. Since 3.5.0 the Home Assistant screen store (5,792 bytes) and the MQTT client with its 768-byte receive buffer are no longer static in either image: `haBoot()` (`src/features/ha/HaBoot.cpp`) takes them from the heap at boot, and only when a broker host is set. A standard device with the host blank therefore keeps that memory too, without the lean image.

That headroom decides whether two fetch paths run at all. `features/radar/RadarClient.cpp` and `features/ticker/StockClient.cpp` both skip their fetch unless a 16,000-byte contiguous block is available, which is what the BearSSL handshake actually allocates. Both refuse quietly rather than crash, so a device below the threshold shows an empty scope or a blank ticker with no error. On a busy LAN, or over a weak link where retransmissions keep the WiFi queues full, the standard build can sit under both.

Read the current figures from `/api/status`: `heap` is free bytes and `maxblk` is the largest contiguous block.

The env inherits `smalltv` through `extends` and interpolates its `build_flags` (`${env:smalltv.build_flags}`) before adding its own, so the board, flash layout, libraries and shared flags stay identical. Four flags do the work: `-D WITH_HA=0`, `-D WITH_USAGE=0` and `-D WITH_THEME=0` drop the features, and `-D SMALLTV_LEAN` selects the matching `UPDATE_ASSET` in `src/config.h` so a lean device self-updates to the lean image rather than the standard one.

### Rolling your own combination

The six `WITH_*` flags in `src/config.h` each default to 1 and can be set to 0 in any env, or passed through `PLATFORMIO_BUILD_FLAGS`:

| Flag | Drops | Static RAM saved on the ESP8266 (2.12.0) |
|------|-------|--------------------------------|
| `WITH_HA=0` | Home Assistant screens over MQTT | 7,668 B |
| `WITH_TICKER=0` | The stock and crypto ticker | 3,328 B |
| `WITH_RADAR=0` | The plane radar | 2,180 B |
| `WITH_USAGE=0` | The Claude usage meter and its mascot | 1,080 B |
| `WITH_THEME=0` | Theme clocks | not measured |
| `WITH_NOTIFY=0` | Notification overlays | not measured |

Each figure is that one flag measured on its own against the 55,536-byte standard build of 2.12.0. Combining flags saves slightly less than the sum, because the features share some code and string data. The Home Assistant figure is lower since 3.5.0, because its screen store left static memory (see above); on a device with the broker host blank that memory is free anyway.

Every feature is guarded at the module level, so dropping one takes its code, its settings handling, and its web UI tab with it. The tab disappears because `/api/config` reports the compiled-in features and the UI hides what is missing, which means a slim build needs no separate web UI.

Two caveats if you publish your own combination. `UPDATE_ASSET` still points at whichever variant your flags select, so a build that is neither standard nor lean will self-update into one of the published images and pick up the features you removed. And the mascot frame data lives in flash rather than RAM, so `WITH_USAGE=0` saves far more flash than heap.

## The smalltv_loader env

`smalltv_loader` (source `src/loader.cpp`) builds a minimal ESP8266 image: WiFi plus a web OTA endpoint at `/update`, nothing else. Its only job is the two-step [SmallTV-ultra install](/NM-SmallTV-Pro/getting-started/flashing/#smalltv-ultra-stock-updater-says-not-enough-space), where the stock Ultra layout rejects the full image. The loader is small enough to fit that stock slot, and it uses this firmware's own 4m1m flash layout (`eagle.flash.4m1m.ld`), so its own `/update` slot is large enough to then accept the full `nm-smalltv-pro-firmware.bin`.

```bash
pio run -e smalltv_loader
```

By default the loader opens an open access point named `SmallTV-Loader` at `192.168.4.1`. To have it auto-join an existing network instead, bake the credentials in at build time by adding the two flags to the env's `build_flags` (or via `PLATFORMIO_BUILD_FLAGS`):

```
-DLOADER_SSID='"MyNet"' -DLOADER_PASS='"secret"'
```

Credentials are compile-time only and never live in the repo. The build output is published as the release asset `nm-smalltv-pro-loader.bin`.

## How one codebase builds for all four

Chip differences are centralized so the feature code stays the same on every board.

- `src/Target.h` checks that the env selected exactly one chip and defines `SMALLTV_FAMILY_ESP32` (C2 and classic ESP32) or `SMALLTV_FAMILY_ESP8266`. Code that differs by chip family tests these two macros, and a build with no chip macro, or two, stops with an error
- `src/Platform.h` holds every chip-specific include, class alias, and small shim. The WiFi stack, web server, HTTPS client, OTA, and reset handling all resolve through it. The ESP32 targets share its Arduino core 3.x branch.
- `src/board_esp8266.h`, `src/board_esp32c2.h`, `src/board_esp32.h`, and `src/board_esp32_pro.h` hold the pin map and panel quirks for each board. `src/config.h` includes the right one based on the build target.
- The three feature modes, the web UI, and the settings layer are identical across all targets.

The target is chosen by a build flag: `SMALLTV_ESP8266`, `SMALLTV_ESP32C2`, or `SMALLTV_ESP32`. The SmallTV Pro defines `SMALLTV_ESP32` (chip-family code paths) plus `SMALLTV_ESP32_PRO` (its pin map and update asset).

In `platformio.ini`, `[common]` holds the source include paths and the libraries every firmware env uses, and `[esp32_base]` holds the single ESP32 platform pin. Each env interpolates them, so a change there reaches every board.

## Project layout

```
src/                    shared core (device, net, web, settings)
  main.cpp              setup/loop and the mode registry
  loader.cpp            the standalone smalltv_loader image (built on its own)
  Target.h              chip check and the SMALLTV_FAMILY_* macros
  Platform.h            per-chip includes, aliases, and shims
  Mode.h                the DisplayMode interface every feature implements
  board_esp8266.h       ESP8266 pin map and panel quirks
  board_esp32c2.h       ESP32-C2 pin map and panel quirks
  board_esp32.h         NM-TV-154 (classic ESP32) pin map and panel quirks
  board_esp32_pro.h     SmallTV Pro (classic ESP32, 8 MB) pin map and panel quirks
  config.h              limits, feature flags, defaults, board selector
  Settings.*            settings struct and LittleFS persistence
  Net.*                 WiFi station, fallback AP, captive portal, mDNS
  WebPortal.*           web server, REST API, OTA endpoint
  webui.h               the single-page UI (HTML/CSS/JS, served from flash)
  Gfx.*                 shared ST7789 core (Arduino_GFX), colour correction
  Clock.*               SNTP and the night-mode window
  WgClient.*            WireGuard tunnel (no-op stubs without SMALLTV_WIREGUARD)
  BearSslTuning.cpp     ESP8266 TLS cipher/curve pinning
  OtaUpdate.*           GitHub self-update (ESP8266)
  features/
    ticker/             TickerMode + StockClient
    usage/              UsageMode + UsageClient + Mascot
    radar/              RadarMode + RadarClient
    notify/             NotifyMode + its overlay frames (armed over HTTP, never persisted)
    ha/                 Home Assistant screens over MQTT
    theme/              the .stheme clock-face engine and its web API
partitions/             Separate C2 and NM-TV-154 4 MB layouts; 8 MB for the Pro
n8n/                    webhook contract and importable workflows
extra_script.py         Windows-only SCons spawn workaround for the ESP8266 envs
```

## Windows builds

`extra_script.py` replaces SCons's process spawn on Windows. The default path sends build-tool command lines through `cmd.exe`, which can mis-tokenize the long quoted lines this project generates and fail a `.cpp` compile with "no such file or directory". The script re-tokenizes the line itself and starts the process directly, including the `\"`-escaped quotes the ESP8266 `ARDUINO_BOARD_ID` define carries.

It is wired into `[env:smalltv]` with `extra_scripts = pre:extra_script.py`, so `smalltv_lean` inherits it through `extends`. The override is gated on `os.name == "nt"` and does nothing on Linux or macOS, where SCons escapes for `sh` with single quotes that this tokenizer does not parse. The ESP32 envs do not use it.

## ESP32 toolchain notes

The ESP32 targets have a few requirements the ESP8266 does not.

- **Platform**: PlatformIO's official espressif32 does not support the C2 and is stuck on Arduino core 2.x. All ESP32 envs use the [pioarduino](https://github.com/pioarduino/platform-espressif32) fork, pinned to one release in `[esp32_base]`, which tracks Arduino core 3.x on ESP-IDF 5.x (`Platform.h`'s ESP32 branch needs core 3.x). The first build downloads a large toolchain and compiles the IDF from source, so it takes several minutes. Later builds are fast.
- **Display driver**: both use `Arduino_HWSPI` with explicit pins. The register-level `Arduino_ESP32SPI` hangs on the C2, and the software-SPI path in the library does not cover it. `Arduino_HWSPI` uses the stock SPI driver and works.
- **Flashing the C2**: uploads call the system esptool, not the one bundled with PlatformIO, which hangs entering download mode on that board. Install it with `pip install esptool`. The classic ESP32 flashes with either.
- **Image size flags**: the ESP32 envs build with `-D CORE_DEBUG_LEVEL=0 -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables` to keep the images inside their OTA slots. The first flag also silences the Arduino core's own debug messages on the serial console; the firmware's serial output is unchanged.
- **Partitions**: the C2 uses `partitions/smalltv_4mb_ota.csv` with two 1,572,864-byte app slots and about 0.9 MB of LittleFS. The NM-TV-154 has its own `partitions/nm_tv_154_4mb_ota.csv` with two 1,966,080-byte app slots and 192 KiB of LittleFS. The SmallTV Pro's 8 MB layout in `partitions/smalltv_8mb_ota.csv` has 2.125 MB app slots and about 3.7 MB of LittleFS, matching the stock table. Changing an installed device's partition table requires a merged factory image over USB, not app-only OTA.

## WireGuard

The optional WireGuard client is compiled into `smalltv_c2`, `smalltv_esp32_8mb` and `smalltv_esp32_wg`. Its switches live in those envs in `platformio.ini`: `-D SMALLTV_WIREGUARD=1` plus `-D CONFIG_WIREGUARD_MAX_PEERS=1` and `-D CONFIG_WIREGUARD_MAX_SRC_IPS=5`, and `droscy/esp_wireguard @ 0.4.5` in `lib_deps`. These have to be `build_flags`, not `build_src_flags`, because they must reach the library's own translation units. Without the flag `src/WgClient.cpp` compiles to no-op stubs, so every other env builds unchanged.

The two peer limits are compile-time because `esp_wireguard` allocates peers statically inside its device struct; both default to 1, and `MAX_SRC_IPS` has to cover every allowed-IPs entry plus the device's own address, which the component adds itself.

### The smalltv_esp32_wg env

`smalltv_esp32_wg` is the full-feature NM-TV-154 image and is published as `nm-smalltv-pro-firmware-esp32-wg.bin`. It includes dual tickers, weather, timers, carousel, themes, notifications, burn-in prevention, usage, radar, Home Assistant and WireGuard. The directly built `smalltv_esp32` variant omits only WireGuard.

Both variants set `WITH_KOREAN_FONT=0`. The renderer keeps the Galmuri ASCII arrays and all font sizes, wrapping, animation and scrolling, but drops the Hangul glyph and codepoint tables; unsupported UTF-8 characters render as `?`. Other boards retain the full Korean font by default.

The dedicated layout is what makes the full image fit. Its OTA slots are 1,966,080 bytes each, with 192 KiB left for LittleFS. The first installation, and migration from the old shared 4 MB layout, must write `firmware.factory.bin` over USB so the new partition table is installed. Later app updates and variant switches use ordinary OTA.

The env inherits `smalltv_esp32` through `extends` and interpolates that env's `build_flags` and `lib_deps` rather than restating them, so anything added to the base env reaches both images. It adds `-D SMALLTV_ESP32_WG`, which selects the matching `UPDATE_ASSET` and `FW_VARIANT` in `src/config.h` so a tunnel device self-updates to the tunnel image and a plain one stays plain. Everything else, including the board pin map, still keys off `SMALLTV_ESP32`.

`custom_sdkconfig` is inherited unchanged, which matters for build times: pioarduino keeps one `sdkconfig.defaults` per project and rebuilds the whole Arduino/IDF framework whenever its hash changes. The two envs hash the same, so building them back to back costs one framework build, not two.

## Footprint

Measured as the flashable `firmware.bin`, which is what an OTA slot has to hold. The NM-TV-154 row is the current local full-feature build; the other rows are historical v3.0.0 reference figures.

| Build | `firmware.bin` | Slot | Used | Free |
|-------|---------------|------|------|------|
| `smalltv` | 795,184 B | 1,044,464 B | 76.1% | 249,280 B |
| `smalltv_lean` | 658,896 B | 1,044,464 B | 63.1% | 385,568 B |
| `smalltv_c2` | 1,441,216 B | 1,572,864 B | 91.6% | 131,648 B |
| `smalltv_esp32_wg` | 1,895,120 B | 1,966,080 B | 96.4% | 70,960 B |
| `smalltv_esp32_8mb` | 1,499,104 B | 2,228,224 B | 67.3% | 729,120 B |

The ESP8266 slot comes from `eagle.flash.4m1m.ld`, the C2's from `partitions/smalltv_4mb_ota.csv`, the NM-TV-154's from `partitions/nm_tv_154_4mb_ota.csv`, and the Pro's from `partitions/smalltv_8mb_ota.csv`. The mascot frame data lives in flash, not the heap.

CI checks every image against its slot on each build (`tools/ci_checks.py size`) and fails the build when an image has less than 8,192 bytes free, so an image that cannot be installed over the air is never published. After a local `pio run`, `python tools/ci_checks.py size --min-free 8192 --allow-missing` runs the same check on the envs you built.

The PC-side usage daemon is a separate repo: [clawdmeter-daemon](https://github.com/giovi321/clawdmeter-daemon).
