# Drone Synth desktop plugin

The Android app's synth engine (`app/src/main/cpp`) as a desktop instrument
plugin: **VST3**, **CLAP**, **AU** (macOS) and a **standalone** app, for Windows,
macOS and Linux. Built with [DPF](https://github.com/DISTRHO/DPF) (ISC licence, free).

It's step 1: the full engine, MIDI and the phone controller protocol, with no custom
UI yet. Hosts show their own generic editor for the 69 parameters.

## What it does

- Plays from host MIDI: notes (each on its exact frame), pitch bend (±2 semitones),
  mod wheel (CC 1, brightens the filter), All Notes/Sound Off (CC 123/120, which
  silences a held drone, as on iOS).
- Follows the **Drone Synth Motion Controller**: the phone app sends each knob as an
  NRPN (number = parameter id, 14-bit value across the parameter's range), exactly
  as the iOS app sends it to the AUv3. See `src/NrpnDecoder.h`. NRPN 16000 silences
  everything. The purchase lives in the phone app; the plugin follows any controller
  for free.
- Parameter ids and ranges match the Android `SynthParam.kt` and iOS
  `ParameterDefinitions`, so the same NRPN numbers work everywhere. The reverb's
  crossover, damping and decays follow its type and size, as in the apps.

## Building

```sh
git submodule update --init --recursive        # fetches DPF into desktop-plugin/dpf
cmake -S desktop-plugin -B desktop-plugin/build -DCMAKE_BUILD_TYPE=Release
cmake --build desktop-plugin/build --config Release --parallel
desktop-plugin/build/dronesynth_tests           # offline tests (no host needed)
```

The plugins land in `desktop-plugin/build/bin/`. Without DPF the build makes just the
tests, which cover the engine, parameters and controller protocol.

To try it on a Mac, copy `DroneSynth.vst3` to `~/Library/Audio/Plug-Ins/VST3/`,
`DroneSynth.component` to `~/Library/Audio/Plug-Ins/Components/`, or
`DroneSynth.clap` to `~/Library/Audio/Plug-Ins/CLAP/`. On Windows the VST3 goes in
`C:\Program Files\Common Files\VST3\` and the CLAP in `C:\Program Files\Common Files\CLAP\`.

**Windows builds:** `.github/workflows/desktop-plugin.yml` builds and tests on
Windows and macOS on every push that touches the plugin or engine, and uploads
`build/bin` as a downloadable artifact. To build locally on Windows, use Visual
Studio 2022 (Desktop C++ workload) and the same CMake commands.

## Using the iPhone as a controller today (Windows)

The iOS app sends over Apple's network MIDI (RTP-MIDI). Windows needs Tobias
Erichsen's free **rtpMIDI** driver to see it: create a session, connect to the
phone, then pick that session as the MIDI input on the plugin's track in the DAW.
On a Mac, connect in Audio MIDI Setup > MIDI Network Setup.

## Not done yet

- A custom UI (the next big piece).
- The ARP/SEQ (iOS's NRPNs 77-86 are ignored for now), the looper and the recorder.
- Telling the host about controller moves: the values change in the plugin and show
  in hosts that poll, but the DAW doesn't record them as automation yet
  (`DroneSynthCore::takeControllerChange` is in place for it).
- Presets, Windows code signing and an installer.
- A direct network listener so phones connect without rtpMIDI (step 2).
