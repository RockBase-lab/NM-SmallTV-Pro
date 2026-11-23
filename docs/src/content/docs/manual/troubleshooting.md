---
title: Troubleshooting
description: Diagnose setup, data, display and update problems.
---

Chapter 13 of the complete [English manual](/NM-SmallTV-Pro/manual/user-manual-en/) and [中文手册](/NM-SmallTV-Pro/manual/user-manual-zh/) gives the current checklist. Start with the symptom and Status diagnostics before resetting the device.

| Symptom | Check first |
| --- | --- |
| Setup page does not open | Stay on the device hotspot; open `http://192.168.4.1`; use the displayed password |
| Connected device cannot be reached | Current router IP, same LAN, guest/client isolation and hostname |
| Wrong weather or time | Shared city coordinates, saved changes, Display timezone and NTP access |
| Unexpected Carousel pages | General checkboxes, app dwell and independent symbol/image/layout rotations |
| Touch leaves Pictures | Next page changes apps; use Home or change the touch assignment |
| Old or low-color pictures | Install the supplied v2 library as well as app OTA |
| Empty radar / usage / HA | Center and feed diagnostics / running daemon / configured broker and published screens |
| Black screen but web UI works | Brightness, night/sun schedule, temporary wake and panel settings |
| Update fails | Correct board, OTA versus factory file, layout and available space |
| Repeated crash | Firmware variant, reset reason and serial log; use a compatible recovery image |

Factory reset deletes settings and leaves installed picture/theme files. App-only OTA preserves files and configuration, including an administrator password. Export settings before major changes. See chapter 12 for backup, USB recovery and the correct update procedure.

An NM-TV-154 boot message `No core dump partition found` alone does not establish a crash. Check reset reason, whether boot completes and whether uptime keeps increasing.
