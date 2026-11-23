# Default stocks implementation plan

**Goal:** Stock 1 works without an API key or a custom server, with Samsung, NVIDIA and Apple prices and historical line charts.

**Design:** Reuse Naver for Samsung 005930 and Yahoo chart endpoints for NVDA/AAPL, including the existing Yahoo host retry. Seed only Stock 1, use 20 chart points over approximately one month, and retain the 120 second refresh and 10 second rotation. Detailed weather stays the boot screen. Stock 2 remains optional.

**Upgrade:** Only loading an old configuration with an empty Stock 1 list installs the defaults. Write a stock defaults version in exported/saved configurations so intentionally empty lists remain empty after saving and rebooting. Keep nonempty custom lists and all other settings. API partial updates must never trigger migration.

**Chart correction:** Naver's one month range must request daily bars rather than monthly bars, so the default curve covers about 20 trading days instead of multiple years.

## Execution

- [x] Add and run native regressions for defaults, stored empty list migration, custom configuration preservation, intentional deletion and serialization.
- [x] Add a regression for the Naver one month request and exercise the actual quote/chart parsers with representative provider payloads.
- [x] Implement the minimal settings changes and daily bar correction. Explain the default sources in Stock 1 settings.
- [x] Run native and browser regressions, regenerate the compressed web page, build standard/WireGuard firmware, and check OTA space.
- [x] Independently review the changes, publish local OTA files with matching ELFs and checksums, and document host API validation versus device verification.
