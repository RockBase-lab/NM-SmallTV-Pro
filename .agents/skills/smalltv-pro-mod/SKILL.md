---
name: smalltv-pro-mod
description: >-
  Comprehensive guide and operations runbook for developing, building, flashing,
  and releasing the custom firmware for the GeekMagic SmallTV-PRO (ESP32 8MB Flash).
  Use whenever working on the SmallTV-PRO codebase, modifying display/candlestick/burn-in logic,
  building firmware with PlatformIO, uploading OTA updates to the device (192.168.0.57),
  or publishing GitHub releases to twoarchi/smalltv-pro-mod.
---

# GeekMagic SmallTV-PRO Custom Firmware Runbook

This skill encapsulates the full engineering knowledge, hardware specifications, firmware architecture, build/flashing procedures, and release processes for the **GeekMagic SmallTV-PRO** custom firmware (`twoarchi/smalltv-pro-mod`).

---

## 1. Hardware Architecture & Environment

| Component | Specification / Setting |
| :--- | :--- |
| **MCU** | ESP32-D0WD-V3 (dual-core Xtensa 240MHz, 320KB SRAM, NO external PSRAM) |
| **Flash** | 8 MB SPI Flash (OTA partition ~2.12 MB, SPIFFS/LittleFS ~1.5 MB) |
| **Display** | 1.54" 240×240 IPS LCD (ST7789V driver, SPI Mode 3, 40MHz, CS=GND, Active-low PWM Backlight GPIO 19) |
| **Touch Button** | Capacitive touch sensor pad on top case (GPIO 32, `touchRead()` with adaptive baseline tracking) |
| **Device IP** | `http://192.168.0.57/` (mDNS fallback: `http://smalltv.local/`) |
| **Repo** | `https://github.com/twoarchi/smalltv-pro-mod` (origin branch: `main`) |
| **PlatformIO Env** | `smalltv_esp32_8mb` |
| **PIO Path** | `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"` |

---

## 2. Core Subsystems & Firmware Logic

### A. Screen Refresh & Burn-in Wiper (`src/BurninWiper.h`, `src/BurninWiper.cpp`)
* **Purpose**: Prevents IPS LCD liquid crystal static charge accumulation and image retention from persistent UI elements.
* **Execution**: Non-blocking state machine in `main.cpp` loop. While active, carousel dwell timer is paused and credited via `burninHeldMs()`.
* **Touch-to-Wake**: Tapping the top capacitive button immediately aborts the wiper and restores the active dashboard.
* **7 Visual Effects**:
  1. `matrix`: Cyberpunk green falling digital rain (random characters, gradient tail).
  2. `rainbow`: 8-color SMPTE color bars smoothly scrolling horizontally (exercises all RGB subpixels).
  3. `starfield`: 3D warp spaceflight stars (high pixel turnover rate).
  4. `black`: Deep Black (clears charges, zero light pollution).
  5. `white`: Pure White (industry-standard liquid crystal wiper).
  6. `snow`: High-frequency retro TV static noise.
  7. `mascot`: Centered animated jumping mascot character.
* **REST Preview API**: `POST /api/burnin/preview` with `{"effect":"matrix","durationSec":5}`.

### B. Dual Stock Candlestick Engine (`src/features/ticker/StockClient.cpp`, `TickerMode.cpp`)
* **Stock 1 (KRX / Naver Finance API)**:
  - **Pre-market handling (08:00~09:00)**: KRX does not execute trades before 09:00 (only order accumulation). Minute API returns 0 items. Fallback gracefully to recent daily candles (`timeframe=day`) clamped with positive prices to prevent 0-price scale explosion.
  - **Market Open (09:00:00)**: Automatically transitions to live intraday minute candles (1m/5m/30m/60m).
  - **Date Separator**: Suppressed for single-day intraday charts (`dateSplitIdx = -1`).
* **Stock 2 (US / Global Yahoo Finance API)**:
  - **Daytime (Closed)**: Displays full clean latest regular/extended session without separator lines (`dateSplitIdx = -1`).
  - **Pre-market (18:00+ KST)**: Automatically segments `[yesterday close tail] | [today pre-market]` with an exact vertical date boundary line at `yestTaken`.

### C. HD 160×160 Animated Notify Engine (`src/features/notify/`)
* **Resolution**: 80×80 matrices rendered with clean integer 2× scaling (`cell = 2`, 160×160 px on the 240×240 IPS panel).
* **Presets**: `phone` (ringing), `mailbox` (raising flag), `sms` (typing bubble), `telegram` (paper airplane), plus 5 emotional stickers (`beer`, `busy`, `cry`, `chicken`, `yahoo`).
* **Webhook**: `POST /api/notify` with `{"anim":"mailbox","title":"TITLE","label":"Message","ttl":5}`.

---

## 3. Standard Procedures & Workflows

### 1. WebUI Editing & Compression
1. Edit source HTML: `src/webui.html`.
2. Regenerate compressed header:
   ```bash
   python tools/gzip_webui.py
   ```
   *(Verifies `webui.html -> gzip -> webui.h` match).*

### 2. Building Firmware
Execute PlatformIO build specifically targeting `smalltv_esp32_8mb`:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e smalltv_esp32_8mb
```
* Output binaries located in `.pio/build/smalltv_esp32_8mb/`:
  - `firmware.bin` (~1.89 MB): Used for Web OTA updates.
  - `firmware.factory.bin` (~1.96 MB): Used for initial USB flashing at `0x0`.

### 3. OTA Upload to Physical Device
Upload binary directly via Python OTA upload script:
```powershell
python "C:\Users\user\.gemini\antigravity\brain\<conv_id>\scratch\ota_upload.py"
```
* Verify device reboot (~6s) and status:
  ```powershell
  curl.exe -s http://192.168.0.57/api/status
  ```

### 4. Updating Backups
Always sync built binaries and documentation to desktop backup directories:
* `C:\Users\user\Desktop\SmallTV_PRO_Backup\1_개인용_Private\`
* `C:\Users\user\Desktop\SmallTV_PRO_Backup\2_배포용_Public_Release\`

### 5. GitHub Release & Git Push
1. Commit & push changes:
   ```bash
   git commit -am "<Commit message>"
   git push twoarchi main
   ```
2. Create release via GitHub CLI:
   ```powershell
   gh release create <tag> \
     "<path_to_public>/firmware_<tag>_public_release.bin" \
     "<path_to_public>/firmware.factory_<tag>_public_release.bin" \
     --repo twoarchi/smalltv-pro-mod \
     --title "<Release Title>" \
     --notes-file "<path_to_public>/GITHUB_RELEASE.md"
   ```

---

## 4. Key Constraints & Safety Rules

1. **Memory Ceiling**: ESP32 has no PSRAM. Do not allocate large frame buffers in DRAM (max free heap ~170KB). Use streaming or procedural drawing directly to `Arduino_GFX`.
2. **Backlight Handling**: Active-low PWM on GPIO 19.
3. **No Private Leaks**: Never include private identities (e.g. `soulmate` or `urbano`) in any public releases, git commits, or documentation.
