# Pictures and mode repair

## Evidence

The live device at 192.168.124.14 has an older picture library whose package sizes differ from the current RGB JPEG library. Three animations fail validation with layers[0].loop. Display channel gains are all 100 and color order/inversion remain at defaults.

The live carousel configuration includes Usage, Radar and HA although General has no checkboxes for them. The mode selector offers only Timer, Carousel, Theme and Pictures. Loading any other saved mode selects Timer, so saving unrelated settings changes the mode. Touch Next permanently overrides automatic dispatch and traverses unchecked carousel apps. An empty carousel continues showing an unchecked app.

## Intended behavior

- Ship and install the full RGB JPEG picture library from the supplied original assets, with version 2.0.0, valid boolean animation loops, reproducible checksums and source color verification. Keep custom packages and device configuration.
- Explain outdated animation packages in plain language and provide an explicit library update tool that validates local packages before writing the device.
- Build single-app modes, timer app choices and carousel checkboxes from the same feature map, including every compiled app. Unsupported saved modes remain unmodified by ordinary saves.
- Carousel shows exactly the selected compiled apps. Next advances only within that selection and resumes its dwell; Home restarts at the first selection. Empty selections show a setup message. Saving restarts the dwell.
- Timer uses valid enabled schedules across midnight and waits for NTP or a schedule instead of showing an arbitrary app. Touch preview expires after 15 seconds. Single-app browsing remains available; Home restores the saved mode.
- Centralize wake on actual page transitions, clear fetch cancellation after button actions, and start the clock when a theme becomes visible.
- Detailed weather is the factory default. Explicitly saved modes and settings survive reboot.

## Verification and delivery

- [x] Add failing native dispatch and real-page configuration regressions.
- [x] Implement mode selection, touch, timer, startup and UI fixes.
- [x] Verify package colors, animation loops, JPEG RGB conversion and installation flow.
- [x] Build standard/WireGuard OTA and factory images, check OTA headroom, and run relevant regressions.
- [x] Review changes, package OTA/ELF/full-color library and migration tool, and verify device state after any authorized deployment.

## Verified results

- Native settings: 78 checks pass; actual dispatch and touch regression passes. Real HTML load/save preserves every mode and unavailable timer apps. All JavaScript regressions pass.
- Ten RGB JPEG pictures, version 2.0.0, plus Classic clocks: 117488 bytes. Native package validation, color counts/source error, animation loops and actual RGB565 callback tests pass. JPEG quality 55 keeps adequate filesystem headroom for online installation.
- HTTP installer tests cover no-write preview, replacement preserving custom pictures/clock/selection, interruption and retry, corrupt local files and insufficient capacity before deletion.
- Both OTA builds pass. Standard 1911024 bytes, free 55056; WireGuard 1956592 bytes, free 9488. The HTML uses optional desktop Zopfli compression with identical decompressed content.
- Factory image generation now depends on each picture source file; both filesystems rebuild when the library changes. The merged factory's filesystem bytes are checked at offset 0x3d0000.
- User authorized direct OTA and serial debugging on COM3. WireGuard OTA returned 200 OK; serial boot completed with Software restart, without panic/assert/backtrace.
- Ten v2 pictures installed and validated on 192.168.124.14; filesystem free space is now 32768 bytes. Original mode, picture, theme, carousel flags and schedules were restored and read back unchanged.
- Device HTML matches the release source byte-for-byte. A complete ten-picture rotation and Weather/Pictures carousel transitions ran without reset. Final check: uptime 208 seconds, free heap 172136 bytes. Physical color appearance still requires viewing the panel.
- Release: dist/nm-tv-154-pictures-modes-20261120, with both OTA/factory/ELF variants, full-color library, standalone updater and SHA256 checksums. All 20 JavaScript regression files passed; six simulated-device installer tests passed.
