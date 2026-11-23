# NMMiner SmallTV Pro / NM-TV-154 Firmware (v3.13.1)

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32%204%2F8MB-red?style=for-the-badge&logo=espressif" alt="ESP32 4/8MB" />
  <img src="https://img.shields.io/badge/Resolution-240%C3%97240%20IPS-blue?style=for-the-badge" alt="240x240 IPS" />
  <img src="https://img.shields.io/badge/Font-Korean%20Galmuri-brightgreen?style=for-the-badge" alt="Korean Galmuri Font" />
  <img src="https://img.shields.io/badge/Notify%20Engine-Multi--Line%20Card-orange?style=for-the-badge" alt="Multi-Line Card" />
  <img src="https://img.shields.io/badge/Screen%20Protection-Burn--in%20Wiper-purple?style=for-the-badge" alt="Burn-in Wiper" />
  <img src="https://img.shields.io/badge/Version-v3.12.0-blue?style=for-the-badge" alt="v3.12.0" />
</p>

Advanced, feature-rich custom firmware for the **NMMiner SmallTV Pro** (classic ESP32, 8 MB flash) and **NMMiner NM-TV-154** (classic ESP32, 4 MB flash), both with a 1.54" 240×240 IPS display.

This fork brings a **native Korean bitmap font engine on the 8 MB Pro**, **multi-line card text wrapping for SMS notifications**, **scheduled app timers with 4 time slots**, **LCD burn-in prevention with 7 procedural & animated refresh effects**, **dual stock market tracking**, and **high-definition 160×160 animated full-screen notifications** to your desk monitor. The NM-TV-154 full-feature build retains every feature with a compact ASCII-only Galmuri font.

---

## 🌟 Key Features

### User manuals / 使用手册

- [中文完整使用手册](docs/src/content/docs/manual/user-manual-zh.md)
- [Complete English user manual](docs/src/content/docs/manual/user-manual-en.md)
- [Product usability review / 产品使用逻辑复盘](docs/product/2026-11-20-product-usability-review.md)

Printable bilingual PDFs are included in the local usability release bundle. These guides cover first connection, General mode selection, independent app/content rotations, optional integrations, backup and recovery.

### Weather clocks and Photo Album (NM-TV-154)

The Weather tab offers four native layouts: simple weather, detailed weather with seconds and sunrise, three-day forecast, and a large two-tone clock. Select a layout or enable automatic rotation (10–3600 seconds). Set the location coordinates and clock timezone before use. Weather comes from Open-Meteo in one request every 10 minutes by default; the last successful reading stays visible after a failed refresh. The free API requires no key for non-commercial use; commercial deployment needs an appropriate Open-Meteo plan. Forecast dates follow the location; clock time follows the configured timezone. NM-TV-154 screen labels use ASCII.

Photo Album supports listing, selecting, refreshing, installing and deleting 240×240 `.stheme` picture packages. Custom picture IDs start with `picture-`. The NM-TV-154 factory pictures preserve RGB color using baseline JPEG tiles decoded by the classic ESP32 ROM with a 4 KiB workspace. GIFs retain two sampled frames to fit the 192 KiB filesystem. Update the picture packages as well as the app when upgrading from the older three-color assets; app-only OTA retains existing pictures. Direct JPG/GIF browser upload remains planned.

Plane Radar has its own settings tab with an explicit radar center, offline city presets, Open-Meteo city search, six-decimal coordinates and independent feed/display controls. Use the weather/device position or choose a separate radar center. Save and show radar switches the device immediately. Four weather-clock layouts use the bundled OFL Bebas Neue font, shaded weather icons and layouts based on the reference photographs.

NM-TV-154's factory default is Detailed weather. An explicitly saved app or automatic mode survives reboot. The sunrise line alternates with the device's IP address every 10 seconds. Its lower-right spaceman uses all 14 frames from `spaceman.gif`, fitted to 56×56 and played at the source's 80 ms frame interval. Animation and IP changes repaint only their own regions. The sprite is embedded in the app, so an app OTA includes it.

See [reference comparison and verification](docs/product/geekmagic-nm-tv-154-optimization-2026-11-20.md) and [rendered clock previews](docs/product/weather-clocks-preview.png).

### 1. Korean Bitmap Font Engine & Multi-Line Card Mode (New in v3.12.0)
* **Native Korean Font Support (8 MB Pro)**: Embedded KS X 1001 2,350 Korean syllables + ASCII + symbols based on GalmuriMono. The 4 MB NM-TV-154 uses the compact ASCII subset.
* **Optimized 22px Scaling**: 11px base with 2× integer scale (22×22 px Korean, 12×22 px ASCII), with 1px letter spacing and 12px line gap for optimal readability.
* **Multi-Line Card Wrap Mode (`wrap: true`)**: Ideal for SMS/text forwarding (e.g. from Android Tasker). Displays title, divider line, and 5~6 lines of clean wrapped text without scrolling.
* **Zero Marquee Smear**: Marquee scrolling uses a 1ms single-burst background wipe without display bus deadlock.
* **100% Backward Compatible**: All existing notifications without `wrap` retain the classic animated icon + 1-line marquee scroll.

### 2. Scheduled App Timers & Simplified Mode Selection
* **4-Slot 24h App Timer**: Schedule exact apps (Stock 1, Stock 2, Weather) to run based on the time of day with automatic cross-midnight resolution.
* **Streamlined Mode Dropdown**: Clean choice between **Timer** (scheduled by time) and **Carousel** (rotate apps on a dwell timer).

### 2. Screen Refresh & Burn-in Prevention Engine
* **LCD Image Retention Protection**: Periodic pixel exercise routines to prevent and reverse IPS LCD image persistence caused by static stock tickers.
* **Dual Scheduling**: Run periodically (Every N minutes, smartly aligned to the clock e.g. `:00` on the hour) or daily at a specific time (`HH:MM`).
* **Instant WebUI Preview**: Test any effect live on screen with a single click.
* **Touch to Wake**: Tapping the top touch button immediately dismisses the refresh screen and returns to your active dashboard.
* **7 Built-in Refresh Modes**:
  * `matrix`: Cyberpunk digital rain
  * `rainbow`: 8-color SMPTE smooth scrolling bars
  * `starfield`: 3D warp spaceflight stars
  * `black`: Deep Black (liquid crystal discharge, zero glare)
  * `white`: Pure White (industry-standard image retention wiper)
  * `snow`: Retro TV static noise
  * `mascot`: SmallTV animated jumping mascot

### 2. Dual Stock Candlestick Tracking
* **Stock 1**: Ready to use Samsung Electronics (`005930`, Naver), NVIDIA (`NVDA`, Yahoo) and Apple (`AAPL`, Yahoo) quotes with one-month line charts, without an API key or custom server. Rotates every 10 seconds and refreshes every 120 seconds. Existing custom lists are preserved; old empty lists receive the defaults on upgrade. Candlestick and intraday chart options remain available.
* **Stock 2 (US / Global)**: Real-time and pre-market quotes via Yahoo Finance API (`SOXX`, `SPY`, `QQQ`, `NVDA`, `AAPL`, `TSM`, etc.). Intelligent multi-session boundary separating yesterday's close from today's live pre-market.
* **Background Pre-fetching**: Data is cached in the background so switching between clock and charts is instantaneous.

### 3. HD 160×160 Animated Notify Engine
* **Clean Integer 2× Scaling**: Source assets are hand-tuned 80×80 pixel matrices rendered at `cell = 2` (160×160 px on the 240×240 panel). Completely eliminates non-integer scaling artifacts, blurriness, and pixel crawling.
* **REST API Webhook**: Simple `/api/notify` JSON endpoint compatible with **Home Assistant**, **n8n**, **Node-RED**, **AI Agents (Claude, AutoGPT)**, and shell scripts.
* **Built-in Animation Presets**:

| Tag (`anim`) | Aliases | Frames & Speed | Description |
| :--- | :--- | :--- | :--- |
| **`phone`** | `ring`, `call` | 10 frames @ 70ms | Retro pink telephone vibrating & ringing |
| **`mailbox`** | `mail` | 16 frames @ 100ms | 3D mailbox raising red flag & opening hatch |
| **`sms`** | `message`, `msg`, `text` | 19 frames @ 100ms | Typing bubble with moving dots and popup |
| **`telegram`** | `tg` | 4 frames @ 100ms | Telegram paper airplane flying & rotating |
| **`beer`** | `beer_chicken` | 1 frame | Beer mug & fried chicken cheer sticker |
| **`busy`** | `busy_coffee` | 1 frame | Working overtime pouring coffee sticker |
| **`cry`** | `crying_bbu` | 1 frame | Crying reaction character sticker |
| **`chicken`** | `cheering_chicken` | 1 frame | Motivational chicken sticker |
| **`yahoo`** | `yahoo_stars` | 1 frame | Celebration stars sticker |

---

## 🚀 REST API Usage

### Screen Refresh Preview (`/api/burnin/preview`)
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"effect":"matrix","durationSec":5}' \
  http://<SMALLTV_IP>/api/burnin/preview
```

### Full-Screen Notification Alert (`/api/notify`)
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"anim":"mailbox","title":"GITHUB","label":"PR #42 merged successfully!","ttl":5}' \
  http://<SMALLTV_IP>/api/notify
```

---

## NM-TV-154 (4 MB) — Full-Feature Image

Use the `smalltv_esp32_wg` target for the complete feature set: dual tickers, weather, timers, carousel, themes, notifications, burn-in prevention, usage meter, radar, Home Assistant and WireGuard.

```bash
pio run -e smalltv_esp32_wg
```

Outputs:

- `.pio/build/smalltv_esp32_wg/firmware.bin` — app image for OTA after the current layout is installed.
- `.pio/build/smalltv_esp32_wg/firmware.factory.bin` — merged bootloader, partition table and app for USB installation.

The dedicated layout provides two 1,966,080-byte OTA slots and 192 KiB of LittleFS. **The first install, and migration from any older NM-TV-154 partition layout, must use the factory image over USB**:

```bash
python -m esptool --chip esp32 --port COM3 --baud 921600 write_flash 0x0 firmware.factory.bin
```

Later updates use `firmware.bin` in the web UI's **System** tab. The NM-TV-154 keeps Galmuri ASCII glyphs and every font size, wrapping, animation and scrolling mode, but omits the Hangul bitmap tables; unsupported UTF-8 characters display as `?`. The 8 MB SmallTV Pro keeps the full Korean font.

---

## 📥 Flashing & Installation

### Routine update over WiFi

1. Export a settings backup from **System**.
2. Choose the app OTA file for the board and intended features. NM-TV-154 uses `smalltv_esp32` or `smalltv_esp32_wg`; SmallTV Pro uses its separate 8 MB target. Never substitute a Pro image for NM-TV-154.
3. Open **System → Manual update (OTA)**, select the matching app `.bin` and press **Upload & flash**.
4. Keep power connected, wait for reboot and reopen the device address. App OTA preserves settings and package files. Old picture libraries need their separate update.

### First install, partition migration or recovery

Use the matching board's factory image through USB/UART. For NM-TV-154 on Windows, an example is:

```bash
python -m esptool --chip esp32 --port COM3 --baud 921600 write_flash 0x0 firmware.factory.bin
```

Confirm the port and board first. Factory installation includes partitions and files and can overwrite stored settings. Never upload a factory image through web OTA.

### GitHub update

The System update card queries the release repository configured in the installed build and selects its matching firmware variant. Locally supplied improvements may not yet exist in that repository. Check the release notes and target filename before using remote update; a local OTA does not publish a GitHub release. See the complete manuals for backup and recovery.

---

## 🛠️ Building from Source

This project uses [PlatformIO](https://platformio.org/).

```bash
git clone https://github.com/RockBase-lab/NM-SmallTV-Pro.git
cd smalltv-pro-mod
pio run -e smalltv_esp32_8mb  # SmallTV Pro, 8 MB
pio run -e smalltv_esp32_wg   # NM-TV-154, all features
```
