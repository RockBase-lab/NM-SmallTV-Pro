# NMMiner SmallTV Pro identity and crash investigation

**Goal:** Apply the user-selected NMMiner SmallTV Pro name, nm-smalltv-pro identity and RockBase-lab/NM-SmallTV-Pro repository; investigate the reported safe-mode CRASH page with real device evidence.

**Architecture:** Keep existing configuration and partition layout. Update firmware/web/documentation identities and the local Git origin together. Keep release asset names consistent with CI. Diagnose the existing 3.13.0 before replacing it. Release the resulting maintenance firmware as 3.13.1, preserving configuration and picture packages.

**Validation:** COM3 at 115200, device status/config readback, focused native regressions for any confirmed crash cause, all webpage regressions, both NM-TV-154 builds and an 8192-byte minimum OTA headroom gate.

- [x] Read device reset/auth state and locate safe-mode, password and update code.
- [x] Capture COM3 while restarting and exercising network/search workloads; distinguish historical crash evidence from a reproduced fault.
- [x] Apply requested identity/repository changes, with consistent firmware/UI/documentation links.
- [x] Improve crash recovery guidance and retained diagnostics. The original panic could not be reproduced; its root cause remains unconfirmed.
- [x] Build, verify, package and deploy the authorized maintenance OTA, retaining configuration and picture library. Apply the requested Chengdu location and verify it survives reboot.
- [x] Record results and exact remaining uncertainty; explain separate web password and setup-hotspot password.

The naming design is explicitly supplied by the user and does not require another naming approval. No commit, push, release publication, factory erase or configuration reset is authorized or needed.
