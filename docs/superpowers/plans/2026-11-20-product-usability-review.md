# Product usability review and bilingual manuals

**Goal:** Make the first-use, everyday, update and recovery paths consistent, then deliver matching Chinese and English user manuals.

**Design:** Retain the existing web tabs and saved user preferences. Use explicit configuration instead of hidden locations, synchronize the two shared-location forms, default Carousel to ready-to-use apps, and describe every save action accurately. Explain application mode, nested rotations and temporary screen overlays separately. Publish maintainable Markdown manuals with printable PDFs and a source-linked review report.

**Scope:** All project capabilities and hardware variants; NM-TV-154 is the primary walkthrough. This is a usability repair, not a navigation redesign. Do not commit, erase settings or change partitions. Existing deployment authorization covers this device's firmware updates; retain a fresh configuration snapshot and verify any new deployment separately.

- [x] Audit setup, saved modes, display controls, data prerequisites, optional integrations, updates and recovery against implementation.
- [x] Reproduce and repair missing-location behavior, coordinate synchronization/zero values, misleading controls/status text and unconfigured default carousel entries with focused regressions.
- [x] Write Chinese and English task-based manuals and a product review with evidence and resolved/deferred findings.
- [x] Generate PDFs, render and inspect their pages, verify links and equivalent coverage.
- [x] Run affected regressions, build both NM-TV-154 variants, check OTA headroom, review fixes and prepare a distinct release.
- [x] Verify the device after any firmware deployment and record remaining physical or external-service limits.

## Completion evidence

Both PDFs: 16 pages, 14 chapter bookmarks, rendered and inspected. All 20 Node regressions, affected native settings/mode/weather suites, auth/sun and DNS callback checks pass. Both NM-TV-154 builds pass; standard/WG OTA headroom 53,888/8,304 bytes. Astro generates 23 pages. Device 192.168.124.14 updated successfully; saved selections and shared clock configuration preserved, live HTML equals source, ten v2 picture packages valid, COM3 boot complete and released, 60-second runtime samples show no unexpected reboot. See docs/product/2026-11-20-product-usability-review.md for scope and limits.

## Manual contract

Both languages cover: quick start and actual hotspot credentials; network/IP discovery; single app/Carousel/Timer; touch defaults and reassignment; weather location/timezone and four layouts; Stock 1/2 and Crypto; Pictures and Theme clocks; Usage, Radar, HA and WireGuard prerequisites; brightness/night/sun/refresh; notifications; configuration backup, OTA versus factory and picture-library updates; troubleshooting and hardware differences. Do not present integrations as working without their external prerequisites or promise live/real-time financial data.

## Evidence found

- WeatherClient silently substitutes Seoul for 0/0; users cannot identify that substitution from the weather page.
- `placePick` updates the Display coordinate fields but leaves Weather coordinate fields stale; manual edits in Weather are not copied until a separate Apply operation. Loading `ck.lat || fallback` misrepresents valid zero coordinates.
- All core Carousel flags initially true, including empty Stock 2, Usage without daemon, Radar without location and HA without broker.
- Status says Mode=STA/AP, which denotes WiFi mode rather than the application selected in General.
- Pictures hint hardcodes short/long actions even though they are configurable; Weather location hint incorrectly promises a reboot.
- Existing README promises Detailed weather on every boot, and older guides point to Display/Mode and the wrong updater file/tab. These contradict current saved-mode semantics and navigation.
