# SmallTV NM searchable configuration implementation plan

**Goal:** Deliver city, timezone, stock and crypto input search with coherent selection/save behavior, SmallTV NM identity and version 3.13.0.

**Architecture:** Existing web UI owns filtering, request scheduling and browser cache. Reuse current selectors and manual rows. Extract testable search logic in a dedicated browser source asset, bundled by the existing gzip generator. Keep any provider proxy fixed-host, authenticated, bounded and opt-in on supporting firmware.

**Tech stack:** Vanilla JavaScript, Arduino/PlatformIO C++, Node regression fixtures, Python packaging and PDF rendering.

- [x] Verify provider responses/CORS and existing identity, selectors, tests and space limits.
- [x] Add failing behavior regressions for offline/online selection, stale responses, composition, filters, active Spot-only directory and source-aware stock identities.
- [x] Implement shared browser search, local timezone filter/aliases, stock and crypto selection with explicit save semantics; validate multilingual and offline behavior.
- [x] Add only the proxy needed by verified provider constraints, with auth, query bounds, fixed hosts, result limits and focused native checks.
- [x] Update SmallTV NM 3.13.0 identity, source bundling/compression, bilingual instructions, PDF edition and release notes; preserve compatibility identifiers.
- [x] Run all web regressions and affected native suites, build both firmware variants, verify OTA headroom, review and render manuals.
- [x] Package a fresh release, deploy the authorized OTA with configuration readback and COM3 observations, and record outcomes and limits.

Work in the current checkout to retain the user's accumulated changes. Do not commit or alter partition layout. The design and implementation are already authorized by the user's request; no further approval gate is needed for routine decisions.
