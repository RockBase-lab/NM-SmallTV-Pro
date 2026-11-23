# Errors

## [ERR-20261119-001] firmware-size-gate

**Logged**: 2026-11-19T15:20:00+08:00
**Priority**: high
**Status**: resolved
**Area**: config

### Summary
NM-TV-154 full-feature firmware linked but failed the actual OTA file-size gate.

### Error
```
smalltv_esp32_wg: 4928 B free in a 1900544 B slot; need at least 8192 B
```

### Context
PlatformIO's linker summary reported 1,891,026 bytes, while the flashable `firmware.bin` was 1,895,616 bytes. The release gate correctly uses the latter.

### Suggested Fix
Share the four duplicated ASCII glyph-rendering loops in the `WITH_KOREAN_FONT=0` path, then rebuild and rerun `tools/ci_checks.py size` against `firmware.bin`.

### Metadata
- Reproducible: yes
- Related Files: src/KoreanFont.cpp, partitions/nm_tv_154_4mb_ota.csv, tools/ci_checks.py

### Resolution
- **Resolved**: 2026-11-19T15:20:00+08:00
- **Notes**: Kept the shared ASCII renderer and enlarged both OTA slots to 1,966,080 bytes, leaving 192 KiB for LittleFS. Final `firmware.bin`: 1,895,120 bytes; gate passed with 70,960 bytes free.

---

## [ERR-20261119-002] host-tests-environment

**Logged**: 2026-11-19T15:20:00+08:00
**Priority**: low
**Status**: pending
**Area**: tests

### Summary
Some host-native tests could not start on this Windows machine.

### Error
```
c++: command not found
UnicodeDecodeError: 'gbk' codec can't decode a UTF-8 documentation file
```

### Context
The C++ test scripts require a desktop compiler (`c++`, `g++` or `clang++`), which is not installed. One Python test also needs UTF-8 mode (`PYTHONUTF8=1`). Firmware builds and Node.js tests do not depend on that desktop compiler.

### Suggested Fix
Run the complete host suite in the existing Linux CI environment, or install a desktop C++ compiler locally and use `PYTHONUTF8=1`.

### Metadata
- Reproducible: yes
- Related Files: tests/theme, tests/ha/run.sh, tests/sun/run.sh

---

## [ERR-20261119-003] smalltv-ultra-image-download

**Logged**: 2026-11-19T16:55:00+08:00
**Priority**: low
**Status**: resolved
**Area**: infra

### Summary
SmallTV-Ultra image download ended before the declared GIF response length.

### Error
```
curl: (18) end of response with 15605 bytes missing
```

### Context
Downloading the Pictures page assets over HTTP/1.1 completed three JPG files, then left `ezgif-3-de29a3740c.gif` incomplete. An HTTP/1.0 probe returned `Connection: close` and the correct length for the first JPG.

### Suggested Fix
Retry all remaining files with curl `--http1.0`, then validate JPEG/GIF structure and dimensions instead of trusting transfer completion alone.

### Metadata
- Reproducible: unknown
- Related Files: docs/product/assets/smalltv-ultra/pictures

### Resolution
- **Resolved**: 2026-11-19T16:56:00+08:00
- **Notes**: Retried the incomplete and remaining assets with HTTP/1.0. All 10 JPEG/GIF files passed Pillow verification and full decoding; dimensions, frame counts, byte sizes, and SHA-256 hashes are recorded in `manifest.json`.

---

## [ERR-20261119-004] nm-tv-154-new-tests

**Logged**: 2026-11-19T17:40:00+08:00
**Priority**: low
**Status**: resolved
**Area**: tests

### Summary
The first run of the new NM-TV-154 tests exposed incorrect test assumptions and incomplete fake-DOM fixtures.

### Error
```
ReferenceError: hideFeat is not defined
AssertionError: 1 != 2 : classic-24
```

### Context
The web settings test loaded a configuration with compiled-out features but did not load `hideFeat()`. The package test read the STH1 entry-count field as though it were the manifest spec version; `classic-24.stheme` correctly contains one entry.

### Suggested Fix
Run the page test with all features enabled and stub only theme loading. Assert the STH1 entry count and reserved field according to `theme_pack.py`; check the manifest's `spec` separately.

### Metadata
- Reproducible: yes
- Related Files: tests/weather/test_webui.js, tests/theme/test_nm_tv_154_assets.py, tools/theme_pack.py

### Resolution
- **Resolved**: 2026-11-19T18:00:00+08:00
- **Notes**: Corrected the STH1 assertions and completed the fake DOM. Normalized CRLF in two source-wiring tests and added the new location fields to the sun fixture. All affected Node and asset tests then passed.

---

## [ERR-20261119-005] esp8266-shared-build-check

**Logged**: 2026-11-19T18:00:00+08:00
**Priority**: low
**Status**: pending
**Area**: config

### Summary
The optional full ESP8266 build compiled the shared changes but did not link because the image exceeded its flash region.

### Error
```
section `.irom0.text' will not fit in region `irom0_0_seg'
```

### Context
This was an additional compatibility check, not an NM-TV-154 target. All shared sources, including the three-server ESP8266 `configTime()` call, compiled. The full ESP8266 target is not in `default_envs`; its resource-limited `smalltv_lean` variant remains the appropriate follow-up for that hardware.

### Suggested Fix
Review ESP8266 full-image flash usage separately, or use the existing lean target. Do not remove NM-TV-154 features to address a different board's limit.

### Metadata
- Reproducible: yes
- Related Files: platformio.ini, src/Platform.h, src/Clock.cpp

---

## [ERR-20261120-001] crypto-webui-test-fixture

**Logged**: 2026-11-20T10:20:00+08:00
**Priority**: low
**Status**: resolved
**Area**: tests

### Summary
The Crypto page test's fake table row did not populate fields from the real row template.

### Error
```
AssertionError: '' !== '1.25'
```

### Context
`addRow()` assigns HTML with `value` attributes, while the initial DOM stand-in returned empty input values regardless of that markup.

### Suggested Fix
Parse the row template's symbol, name, quantity and cost value attributes in the fake row's `innerHTML` setter.

### Metadata
- Reproducible: yes
- Related Files: tests/mode/test_webui_crypto.js

### Resolution
- **Resolved**: 2026-11-20T10:21:00+08:00
- **Notes**: Added value-attribute parsing to the test-only DOM stand-in.

---

## [ERR-20261120-002] platformio-esptool-install

**Logged**: 2026-11-20T10:20:00+08:00
**Priority**: medium
**Status**: resolved
**Area**: infra

### Summary
PlatformIO installed a broken editable esptool 5.3.0 package and could not build the bootloader image.

### Error
```
ERROR: Support for platform 'Windows-' hasn't been added yet.
ModuleNotFoundError: No module named 'esptool'
```

### Context
All project translation units compiled, but the build stopped at `bootloader.bin`. The system Python 3.11 already has a working esptool 5.2.0 installation.

### Suggested Fix
Expose the existing system esptool on the build's `PYTHONPATH`, then rerun the same firmware build.

### Metadata
- Reproducible: yes
- Related Files: .pio/core/penv, platformio.ini

### Resolution
- **Resolved**: 2026-11-20T10:25:00+08:00
- **Notes**: Added the existing Python 3.11 site-packages to PYTHONPATH so the build used esptool 5.2.0. Both NM-TV-154 firmware variants built and passed image validation.

---

## [ERR-20261120-003] webui-check-windows-encoding

**Logged**: 2026-11-20T11:45:00+08:00
**Priority**: low
**Status**: resolved
**Area**: tests

### Summary
Extracting the UTF-8 web UI script on Windows failed before the JavaScript syntax check ran.

### Error
```
UnicodeDecodeError: 'gbk' codec can't decode byte 0x80 in position ...
```

### Context
`Path.read_text()` used Windows' default GBK encoding. The HTML contains Chinese translations and other UTF-8 characters.

### Suggested Fix
Specify `encoding='utf-8'` for both reading the HTML and writing the extracted JavaScript, or run Python with `PYTHONUTF8=1`.

### Metadata
- Reproducible: yes
- Related Files: src/webui.html, tmp/webui-check.js

### Resolution
- **Resolved**: 2026-11-20T11:46:00+08:00
- **Notes**: Re-ran extraction with explicit UTF-8 input and output; `node --check` then passed.

---

## [ERR-20261120-004] fullscreen-assets-littlefs-space

**Logged**: 2026-11-20T12:03:00+08:00
**Priority**: medium
**Status**: resolved
**Area**: config

### Summary
The first 240×240 four-colour asset set fit the raw partition byte estimate but failed actual LittleFS image creation.

### Error
```
Error building filesystem image: LittleFSError -28: LFS_ERR_NOSPC
```

### Context
Twelve packages occupied 168,460 B in the 196,608 B partition. Raw remaining bytes do not account for LittleFS metadata and block allocation overhead.

### Suggested Fix
Keep all source pictures and 240×240 dimensions, but use a three-colour, non-dithered palette to improve STI2 run-length compression. Validate by running the real PlatformIO filesystem target, not only by summing file sizes.

### Metadata
- Reproducible: yes
- Related Files: tools/build_nm_tv_154_assets.py, partitions/nm_tv_154_4mb_ota.csv, tests/theme/test_nm_tv_154_assets.py

### Resolution
- **Resolved**: 2026-11-20T12:05:00+08:00
- **Notes**: Three colours reduced the same 12 packages to 141,460 B. Both variants built LittleFS and factory images successfully; the asset regression budget is now 147,456 B.

---
