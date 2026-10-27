# iOS changes to port to Android

Changes made in the iOS app (`~/Documents/APPS/DroneSynth`, branch
`feature/android-parity`) that Android doesn't have yet. Delete each entry once it's
ported. (The other direction is `ANDROID-CHANGES-TO-PORT.md` in the iOS repo.)

## 2026-10-27

Ported or checked on 2026-10-28; nothing left to port from this batch.
- Keyboard clicks: tested on the Moto (Android 10). Zero underruns while the keyboard
  opened, closed and was typed into with a drone playing; with `adjustNothing`,
  Android 10 reports no keyboard inset, so the screen doesn't re-lay out at all.
  Not ported.
- Cutoff at 12 o'clock (~894 Hz) with double-tap reset: ported.
- Tighter LFO, TUNE, Delay and ARP drawers: ported.
