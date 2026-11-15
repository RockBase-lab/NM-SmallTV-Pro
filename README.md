# GeekMagic SmallTV-PRO Modded Firmware (v3.9.9)

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32%208MB-red?style=for-the-badge&logo=espressif" alt="ESP32 8MB" />
  <img src="https://img.shields.io/badge/Resolution-240%C3%97240%20IPS-blue?style=for-the-badge" alt="240x240 IPS" />
  <img src="https://img.shields.io/badge/Notify%20Engine-160%C3%97160%20HD-green?style=for-the-badge" alt="160x160 HD" />
  <img src="https://img.shields.io/badge/Version-v3.9.9-orange?style=for-the-badge" alt="v3.9.9" />
</p>

An advanced, feature-rich custom firmware for the **GeekMagic SmallTV-PRO** desktop cube (classic ESP32 with 8 MB Flash and 1.54" 240×240 IPS display).

This fork brings **high-definition 160×160 animated full-screen notifications**, **dual stock market tracking (Asian/KRX + US candlesticks)**, and **customizable refresh intervals** to your desk monitor.

---

## 🌟 Key Features

### 1. HD 160×160 Animated Notify Engine
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

### 2. Dual Stock Candlestick Tracking
* **Stock 1 (Domestic / KRX)**: Real-time price quotes, price changes, and full candlestick / line charts powered by Naver Finance API (1-min, 5-min, 30-min, 60-min, Daily, Weekly intervals).
* **Stock 2 (US / Global)**: Real-time and pre-market quotes via Yahoo Finance API (supports indices and equities: `SOXX`, `SPY`, `QQQ`, `NVDA`, `AAPL`, `TSM`, etc.) with candlestick zoom.
* **Background Pre-fetching**: Data is cached in the background so switching between clock and charts is instantaneous.

### 3. Configurable WebUI
* **Weather Tab**: Direct control over weather polling intervals (in minutes).
* **General Tab**: Customizable automatic carousel cycling interval and individual app visibility toggles.

---

## 🚀 REST API Usage (`/api/notify`)

Trigger full-screen animated alerts by sending an HTTP POST request:

* **Endpoint**: `http://<SMALLTV_IP>/api/notify`
* **Method**: `POST`
* **Headers**: `Content-Type: application/json`

### JSON Parameters

| Field | Type | Required | Description |
| :--- | :--- | :--- | :--- |
| `anim` | string | **Yes** | Animation identifier (e.g. `phone`, `mailbox`, `sms`, `telegram`, `beer`, `busy`, `cry`) |
| `title` | string | No | Top headline text (ASCII/alphanumeric recommended) |
| `label` | string | No | Bottom message text (automatically scrolls if longer than screen width) |
| `ttl` | number | No | Display duration in seconds (default: 5) |

### Example (cURL)
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"anim":"mailbox","title":"GITHUB","label":"PR #42 merged successfully!","ttl":5}' \
  http://192.168.0.57/api/notify
```

### Example (PowerShell)
```powershell
Invoke-RestMethod -Uri "http://192.168.0.57/api/notify" -Method Post -ContentType "application/json" `
  -Body '{"anim":"phone","title":"CALL","label":"Incoming call from Support","ttl":5}'
```

---

## 📥 Flashing & Installation

### Option A: Web OTA Update (Recommended)
1. Download `firmware_v3.9.9_public_release.bin` from the [Releases](https://github.com/twoarchi/smalltv-pro-mod/releases) page.
2. Open your SmallTV WebUI (`http://<DEVICE_IP>/`) in a browser.
   * *⚠️ Note: Disable browser auto-translation (e.g. Chrome Google Translate) before opening the page.*
3. Go to the **System** or **Update** tab.
4. Select `firmware_v3.9.9_public_release.bin` and upload.
5. The device reboots automatically in ~5 seconds. Existing Wi-Fi credentials and saved stock tickers will be preserved.

### Option B: USB Flashing with esptool (Clean Install / Recovery)
If flashing from scratch or recovering a bricked device:

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash 0x0 firmware.factory_v3.9.9_public_release.bin
```

---

## 🛠️ Building from Source

This project uses [PlatformIO](https://platformio.org/).

1. Clone this repository:
   ```bash
   git clone https://github.com/twoarchi/smalltv-pro-mod.git
   cd smalltv-pro-mod
   ```
2. Build for SmallTV-PRO (ESP32 8MB Flash):
   ```bash
   pio run -e smalltv_esp32_8mb
   ```
3. Compiled binaries will be located at:
   - OTA binary: `.pio/build/smalltv_esp32_8mb/firmware.bin`
   - Factory binary: `.pio/build/smalltv_esp32_8mb/firmware.factory.bin`

---

## 📜 Credits & License
* Based on the original open-source firmware by [giovi321/smalltv-mod](https://github.com/giovi321/smalltv-mod).
* Released under the WTFPL / MIT License.
