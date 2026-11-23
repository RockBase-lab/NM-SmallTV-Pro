---
title: Settings explained
description: Find the right tab and understand what saving changes.
---

Use the complete [English manual](/NM-SmallTV-Pro/manual/user-manual-en/) or [中文手册](/NM-SmallTV-Pro/manual/user-manual-zh/) for the current settings reference, examples and prerequisites.

| Tab | Purpose |
| --- | --- |
| Status | Network, firmware, runtime and diagnostics |
| General | Display mode, Carousel, Timer and screen refresh |
| Stock 1 / Stock 2 / Crypto | Symbols, sources, charts and content intervals |
| Weather | Shared device location, units and four clock layouts |
| Display | Brightness, colors, orientation, timezone, schedules, touch, themes and MQTT |
| Pictures | Installed full-screen picture and animation packages |
| Plane Radar | Radar center, feed, radius and display options |
| Usage | Computer-supplied usage data |
| WiFi | Networks, hostname, setup hotspot and optional WireGuard |
| System | Backup, password, firmware update, reboot and reset |

Unavailable features are hidden for the installed build. The **Network mode** in Status means STA/AP, not the app chosen in General.

**Save settings** applies pending edits across tabs. Visiting a tab does not change the displayed app. Use theme, Show picture, weather layout shortcuts and Save and show radar also switch the app. Package installation/removal changes files immediately. WiFi Save & connect reboots.

Passwords left blank in ordinary edits are retained. Restoring an exported settings backup replaces settings and reboots. Exported JSON contains secrets, but does not include picture/theme files or the separate HA screen store.
