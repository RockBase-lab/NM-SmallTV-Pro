---
title: Which release file to download
description: Every firmware asset attached to a release, what each name means, and which one your board needs.
---

This fork's CI publishes five files: the SmallTV Pro app and factory images, the full-feature NM-TV-154 WireGuard app and factory images, and the ESP8266 loader. The table below also records the names used by directly built variants. Read each name in three parts: the board, whether it is an app or factory image, and which feature set it carries.

Get published files from this fork's [Releases page](https://github.com/RockBase-lab/NM-SmallTV-Pro/releases). Untagged builds of the same five files are on its [Actions tab](https://github.com/RockBase-lab/NM-SmallTV-Pro/actions) under the latest `build` run.

## Pick by board, then by install method

| File | Board | Use it for |
|------|-------|-----------|
| `nm-smalltv-pro-firmware.bin` | SmallTV, SmallTV-ultra and SDPro (ESP8266) | The normal install and every later update |
| `nm-smalltv-pro-firmware-lean.bin` | Same ESP8266 boards | The same device when it needs more heap. Home Assistant screens, the usage meter and theme clocks are compiled out |
| `nm-smalltv-pro-loader.bin` | SmallTV-ultra and SDPro (ESP8266) | One-time first install when the stock updater rejects the full image |
| `nm-smalltv-pro-firmware-c2.bin` | SmallTV (ESP32-C2 / ESP8684) | Updates, once this firmware is already running |
| `nm-smalltv-pro-firmware-c2.factory.bin` | SmallTV (ESP32-C2 / ESP8684) | The first install, over USB-C |
| `nm-smalltv-pro-firmware-esp32.bin` | NM-TV-154 (classic ESP32) | Directly built variant without WireGuard; OTA on the current layout |
| `nm-smalltv-pro-firmware-esp32.factory.bin` | NM-TV-154 (classic ESP32) | Directly built variant, for USB installation or partition migration |
| `nm-smalltv-pro-firmware-esp32-wg.bin` | NM-TV-154 (classic ESP32) | Published full-feature image with WireGuard; OTA on the current layout |
| `nm-smalltv-pro-firmware-esp32-wg.factory.bin` | NM-TV-154 (classic ESP32) | The same full-feature image, for first install or old-layout migration over USB |
| `nm-smalltv-pro-firmware-esp32-pro.bin` | SmallTV Pro (classic ESP32, 8 MB) | The first install over the stock web UI, and every later update |
| `nm-smalltv-pro-firmware-esp32-pro.factory.bin` | SmallTV Pro (classic ESP32, 8 MB) | A direct install or a recovery over the internal UART header |

Not sure which board you have? [Hardware and variants](/NM-SmallTV-Pro/getting-started/hardware/) has photos and the tell-tale signs for each.

## How to read a file name

Every name follows the same pattern:

```
smalltv-mod-<image>[-<target>][.factory].bin
```

The `<image>` part is `firmware` for the real thing and `loader` for the minimal ESP8266 installer.

The `<target>` suffix names the board. No suffix at all means the original ESP8266:

| Suffix | Board |
|--------|-------|
| none | SmallTV, SmallTV-ultra and SDPro (ESP8266), all features |
| `-lean` | The same ESP8266 boards, without Home Assistant screens, the usage meter or theme clocks |
| `-c2` | SmallTV with the ESP32-C2 / ESP8684 chip |
| `-esp32` | NM-TV-154, classic ESP32, 4 MB flash |
| `-esp32-wg` | The same NM-TV-154, with the WireGuard client compiled in |
| `-esp32-pro` | SmallTV Pro, classic ESP32, 8 MB flash |

`.factory` marks a merged image. It contains the bootloader, the partition table, and the app, and it gets written to flash offset `0x0` over a cable. A name without `.factory` is an app image: just the application, sized to drop into an OTA slot. The ESP32 boards need the factory image for their first install and the app image for updates after that. The ESP8266 needs no factory variant, because its single image already carries everything.

## Check what a device is running

The System tab shows the running variant next to the version, for example `smalltv-mod 2.12.0 (esp8266-lean)`. The same string is in `/api/status` as `variant`.

That name also decides what a self-update downloads. Each build is compiled with its own `UPDATE_ASSET` and `FW_VARIANT` (`src/config.h`), and the updater only accepts the release asset whose name matches. So a lean device fetches `nm-smalltv-pro-firmware-lean.bin` and stays lean, and an `esp32-wg` device fetches `nm-smalltv-pro-firmware-esp32-wg.bin` and keeps its tunnel. A self-update never moves a device between variants, in either direction: it will not quietly take features away, and it will not quietly add them.

Two boards have a second variant to stay within: the ESP8266 (`esp8266` and `esp8266-lean`) and the NM-TV-154 (`esp32` and `esp32-wg`). Every other board has one image and one variant.

## Switch a device from one variant to the other

Crossing between variants is deliberate, and it is a manual upload rather than a self-update. Download the other `.bin` for the same board and upload it in the System tab. The two images in a pair share a partition table, so the swap is an ordinary OTA into the same slot. On the NM-TV-154 this applies once the current dedicated layout is installed; migration from an older layout first requires the factory image over USB.

Settings survive it. Both images in a pair read the same `config.json` from the same LittleFS partition, and neither touches the layout. A WireGuard configuration survives a round trip in particular: the settings store carries the `wg` block whether or not the client is compiled into the running image, so a device moved from `esp32-wg` to `esp32` and back finds the tunnel set up exactly as it was left. From 3.5.0 the same holds for saves made on the lean image: a save there leaves the carousel slots of the features lean lacks, and a stored mode it cannot show, as they were, so they are still set when the device goes back to the standard image.

One thing does not survive, and it is on the ESP8266 pair only. The lean image has no Home Assistant module, so screens already pushed to the device are dropped and their MQTT topics are no longer subscribed. Your retained messages on the broker are untouched, so going back to the standard image and letting it resubscribe brings the screens back.

After the swap the System tab reports the new variant, and from then on self-update follows the new one.

## Why the ESP8266 and the NM-TV-154 get two images and the other boards get one

The ESP8266 shares a single 80 KB DRAM arena between static allocations and the heap, so anything compiled in costs heap that TLS then cannot have. Two fetch paths refuse to start a handshake when memory runs short: the plane radar and a cash.ch quote each want a 16,000-byte contiguous block. Below that the affected screen goes quiet, and since 2.12.1 the Status tab's Radar line says so explicitly.

The lean image gives that back. On the 3.5.0 build static RAM is 56,984 bytes for the standard image and 48,820 for lean, so the heap starts 8,164 bytes larger. Lean compiles out Home Assistant, the usage meter and theme clocks.

Since 3.5.0 the standard image also leaves out the largest piece of Home Assistant when it is not used: the four-screen store (5,792 bytes) and the MQTT client with its 768-byte receive buffer come from the heap at boot, and only when a broker host is set. On a standard ESP8266 running the Pixel Room theme, free heap is about 14.4 KB with the broker host blank and about 7.3 KB with Home Assistant on. So a device that does not use Home Assistant needs the lean image only when it also has no use for theme clocks and still runs short.

The ESP32 boards have several times the RAM and manage TLS buffers dynamically through mbedTLS, so RAM never forces a second image on them.

Flash still matters on the NM-TV-154. Its dedicated 4 MB layout gives each OTA slot 1,966,080 bytes and leaves 192 KiB for LittleFS. The full-feature WireGuard image fits by omitting only the large Hangul bitmap tables while retaining the Galmuri ASCII glyphs and every text/layout feature. The optional plain image omits WireGuard instead and keeps more flash headroom. Both use the same new table, but moving a device from an older layout first requires a merged factory image over USB; app-only OTA cannot change partitions.

See [Building from source](/NM-SmallTV-Pro/reference/building/#the-smalltv_lean-env) to build either ESP8266 image yourself, or a slimmer combination of your own, and [the smalltv_esp32_wg env](/NM-SmallTV-Pro/reference/building/#the-smalltv_esp32_wg-env) for the NM-TV-154 pair.
