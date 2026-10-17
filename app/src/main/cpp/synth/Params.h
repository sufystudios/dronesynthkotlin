//
// Parameter IDs shared with the Kotlin side (SynthParam.kt).
//
// The numbering deliberately matches the iOS app's ParameterAddress so both
// platforms describe a patch the same way. IDs are permanent once shipped:
// never renumber or reuse one. Android-only controls start at 100.
//

#ifndef DRONESYNTH_PARAMS_H
#define DRONESYNTH_PARAMS_H

namespace synth {

enum ParamId : int {
    // Filter
    kCutoff = 0,
    kLeftResonance = 1,
    kRightResonance = 2,
    kStereoFiltersEnabled = 3,

    // Filter envelope
    kFilterEnvModEnabled = 4,
    kFilterEnvModAmount = 5,

    // Envelope
    kAttack = 6,
    kDecay = 7,
    kRelease = 8,
    kSustain = 9,
    kGlideTime = 10,
    kDroneMode = 11,

    // Oscillators: each slot is 8 consecutive IDs starting at kOsc1Waveform,
    // kOsc2Waveform and kOsc3Waveform, in this order.
    kOsc1Waveform = 12,
    kOsc2Waveform = 20,
    kOsc3Waveform = 28,
    kOscWaveformOffset = 0,
    kOscVolumeOffset = 1,
    kOscOctaveOffset = 2,
    kOscSemitoneOffset = 3,
    kOscFineOffset = 4,
    kOscVibratoEnabledOffset = 5,
    kOscVibratoRateOffset = 6,
    kOscVibratoDepthOffset = 7,

    kOsc3SyncToOsc1 = 36,

    // LFOs: each is 5 consecutive IDs (shape, rate, randomize, enabled, depth).
    kFilterLfoShape = 37,
    kPitchLfoOsc12Shape = 42,
    kPitchLfoOsc3Shape = 47,
    kLfoShapeOffset = 0,
    kLfoRateOffset = 1,
    kLfoRandomizeOffset = 2,
    kLfoEnabledOffset = 3,
    kLfoDepthOffset = 4,

    // Reverb (52...56 are retired iOS delay addresses - never reuse)
    kReverbType = 57,
    kReverbLowCutoff = 58,
    kReverbDamping = 59,
    kReverbLowDecay = 60,
    kReverbHighDecay = 61,
    kReverbSize = 62,
    kReverbPredelay = 63,
    kReverbMix = 64,
    kReverbBody = 65,
    kReverbShimmer = 66,

    // Vintage delay
    kDelayTime = 67,
    kDelaySubdivide = 68,
    kDelayTimeLeft = 69,
    kDelayTimeRight = 70,
    kDelayFeedback = 71,
    kDelayWet = 72,
    kDelayDry = 73,
    kDelayWidth = 74,
    kDelayMixMode = 75,
    kDelayMedium = 76,

    // Android-only
    // 100, 101: the tube stage (retired - never reuse)

    // Performance controls (MIDI) - live state, never stored in presets.
    kPitchBend = 110,  // -1...1, scaled to ±2 semitones
    kModWheel = 111,   // 0...1, brightens the filter

    kNumParams = 128,
};

} // namespace synth

#endif //DRONESYNTH_PARAMS_H
