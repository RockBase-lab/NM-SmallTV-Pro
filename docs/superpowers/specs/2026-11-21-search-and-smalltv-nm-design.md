# SmallTV NM 3.13.0: searchable configuration

The user approved the hybrid search design on 2026-11-21. Implement common choices, browser-side input filtering and online discovery with manual-entry fallback. Product name: SmallTV NM; firmware identifier: smalltv-nm; version: 3.13.0. Preserve existing hostnames, settings, MQTT topics and update repository identity. Historical release filenames remain compatible; new local release is explicitly versioned.

Cities share a 350 ms, composition-aware search across Weather, Display and Radar. Common/offline cities and recent choices precede online Open-Meteo results. Results identify country/region; selecting a result fills fields but never saves automatically. Ignore stale responses and clearly distinguish loading, empty and unavailable states.

Timezone selection remains a native accessible select with a local filter input. Search aliases include localized city names and IANA identifiers. Only offer mappings with device-compatible POSIX rules; preserve custom stored mappings. Selecting a city follows a supported timezone or explicitly asks the user to check it. Do not use current UTC offsets as substitutes for DST rules.

Crypto combines curated common aliases with a browser-cached Binance Spot exchange directory. Only active spot pairs qualify; result identity includes source, pair and asset name where known. Stock discovery combines curated bilingual aliases and provider-specific online search. Verify CORS before choosing browser access; if unavailable, use a bounded authenticated device lookup for supported builds rather than an untrusted public CORS relay. Never equate lookup success with successful quote retrieval. Choosing a market result adds an unsaved row with source and symbol; existing row editing remains available.

All search content is browser-side wherever practical. Cache failures are non-fatal; storage use is bounded. Pending query cancellation and request deadlines avoid stuck or out-of-order results. Search requests must not trigger settings writes. Protect credentials, validate and bound any device proxy, and preserve fallback use offline.

Firmware has very little spare OTA space. Keep the current partition and all existing features; reduce generated web asset overhead and verify both targets exceed the existing 8192-byte headroom gate. Update product-facing identity, docs and printable manuals while retaining attribution and real repository links. Deliver a distinct release with checksums; existing OTA authorization applies to 192.168.124.14 and COM3 verification.
