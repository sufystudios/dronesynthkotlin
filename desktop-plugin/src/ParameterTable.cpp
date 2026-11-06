#include "ParameterTable.h"

#include <cmath>

namespace dronesynth {

namespace {

const char *const kWaveforms[] = {"Sine", "Triangle", "Square", "Saw", "Pink Noise"};
const char *const kLfoShapes[] = {"Sine", "Triangle", "Square", "Random"};
const char *const kReverbTypes[] = {"Room", "Hall", "Plate", "Cathedral"};
const char *const kDelayMixModes[] = {"Stereo", "Ping-Pong", "L then R", "R then L"};
const char *const kDelayMedia[] = {"Plain", "Tape", "Old Tape"};

constexpr ParamKind Lin = ParamKind::Linear;
constexpr ParamKind Log = ParamKind::Log;
constexpr ParamKind Int = ParamKind::Integer;
constexpr ParamKind Bool = ParamKind::Boolean;
constexpr ParamKind Choice = ParamKind::Choice;

#define CHOICES(list) list, int(sizeof(list) / sizeof(list[0]))

// The cutoff's default is halfway along its log sweep (knob at 12 o'clock), ~894 Hz.
const float kCutoffDefault = 40.0f * std::sqrt(20000.0f / 40.0f);

// Hall's reverb settings (ReverbKind.Hall in SynthParam.kt) at size 0.5.
const float kHallLowDecay = std::pow(8.0f, 0.5f);
const float kHallHighDecay = std::pow(7.0f, 0.5f);

const ParamInfo kTable[] = {
    // Oscillator 1
    {12, "osc1Waveform", "OSC 1 Waveform", "Wave 1", "", 0, 4, 3, Choice, CHOICES(kWaveforms)},
    {13, "osc1Volume", "OSC 1 Volume", "Vol 1", "", 0, 1, 0.8f, Lin},
    {14, "osc1Octave", "OSC 1 Octave", "Oct 1", "oct", -3, 3, 0, Int},
    {15, "osc1Semitone", "OSC 1 Semitone", "Semi 1", "st", -12, 12, 0, Int},
    {16, "osc1Fine", "OSC 1 Fine", "Fine 1", "ct", -100, 100, 0, Lin},
    {17, "osc1VibratoEnabled", "OSC 1 Vibrato", "Vib 1", "", 0, 1, 0, Bool},
    {18, "osc1VibratoRate", "OSC 1 Vibrato Rate", "VibRt 1", "Hz", 0.05f, 20, 4, Log},
    {19, "osc1VibratoDepth", "OSC 1 Vibrato Depth", "VibDp 1", "", 0, 1, 0.2f, Lin},

    // Oscillator 2
    {20, "osc2Waveform", "OSC 2 Waveform", "Wave 2", "", 0, 4, 3, Choice, CHOICES(kWaveforms)},
    {21, "osc2Volume", "OSC 2 Volume", "Vol 2", "", 0, 1, 0.35f, Lin},
    {22, "osc2Octave", "OSC 2 Octave", "Oct 2", "oct", -3, 3, 0, Int},
    {23, "osc2Semitone", "OSC 2 Semitone", "Semi 2", "st", -12, 12, 0, Int},
    {24, "osc2Fine", "OSC 2 Fine", "Fine 2", "ct", -100, 100, 8, Lin},
    {25, "osc2VibratoEnabled", "OSC 2 Vibrato", "Vib 2", "", 0, 1, 0, Bool},
    {26, "osc2VibratoRate", "OSC 2 Vibrato Rate", "VibRt 2", "Hz", 0.05f, 20, 4, Log},
    {27, "osc2VibratoDepth", "OSC 2 Vibrato Depth", "VibDp 2", "", 0, 1, 0.2f, Lin},

    // Oscillator 3 (its pitch knob sweeps ±700 cents)
    {28, "osc3Waveform", "OSC 3 Waveform", "Wave 3", "", 0, 4, 0, Choice, CHOICES(kWaveforms)},
    {29, "osc3Volume", "OSC 3 Volume", "Vol 3", "", 0, 1, 0, Lin},
    {30, "osc3Octave", "OSC 3 Octave", "Oct 3", "oct", -3, 3, 1, Int},
    {31, "osc3Semitone", "OSC 3 Semitone", "Semi 3", "st", -12, 12, 0, Int},
    {32, "osc3Fine", "OSC 3 Pitch", "Pitch 3", "ct", -700, 700, 0, Lin},
    {33, "osc3VibratoEnabled", "OSC 3 Vibrato", "Vib 3", "", 0, 1, 0, Bool},
    {34, "osc3VibratoRate", "OSC 3 Vibrato Rate", "VibRt 3", "Hz", 0.05f, 20, 4, Log},
    {35, "osc3VibratoDepth", "OSC 3 Vibrato Depth", "VibDp 3", "", 0, 1, 0.2f, Lin},
    {36, "osc3SyncToOsc1", "OSC 3 Sync to OSC 1", "Sync 3", "", 0, 1, 0, Bool},

    // Filter
    {0, "cutoff", "Cutoff", "Cutoff", "Hz", 40, 20000, kCutoffDefault, Log},
    {1, "leftResonance", "Left Resonance", "Res L", "", 0, 0.99f, 0.5f, Lin},
    {2, "rightResonance", "Right Resonance", "Res R", "", 0, 0.99f, 0.5f, Lin},
    {3, "stereoFiltersEnabled", "Stereo Filters", "Stereo", "", 0, 1, 0, Bool},
    {4, "filterEnvelopeModEnabled", "Filter Envelope", "Flt Env", "", 0, 1, 0, Bool},
    {5, "filterEnvelopeModAmount", "Filter Envelope Amount", "Env Amt", "", 0, 1, 0.5f, Lin},

    // Envelope
    {6, "attack", "Attack", "Attack", "s", 0.002f, 10, 0.1f, Log},
    {7, "decay", "Decay", "Decay", "s", 0.002f, 10, 0.1f, Log},
    {9, "sustain", "Sustain", "Sustain", "", 0, 1, 1, Lin},
    {8, "release", "Release", "Release", "s", 0.002f, 10, 0.1f, Log},
    {10, "glideTime", "Glide", "Glide", "s", 0, 2, 0.1f, Lin},
    {11, "isDroneMode", "Drone Mode", "Drone", "", 0, 1, 0, Bool},

    // LFO: filter
    {37, "filterLFOShape", "Filter LFO Shape", "FLFO Shp", "", 0, 3, 3, Choice, CHOICES(kLfoShapes)},
    {38, "filterLFORate", "Filter LFO Rate", "FLFO Rt", "Hz", 0.05f, 20, 2, Log},
    {39, "filterLFORandomize", "Filter LFO Random", "FLFO Rnd", "", 0, 1, 0.3f, Lin},
    {40, "filterLFOEnabled", "Filter LFO", "FLFO", "", 0, 1, 0, Bool},
    {41, "filterLFODepth", "Filter LFO Depth", "FLFO Dp", "", 0, 1, 0.4f, Lin},

    // LFO: pitch, OSC 1 & 2
    {42, "pitchLFOOsc12Shape", "OSC 1&2 LFO Shape", "PLFO Shp", "", 0, 3, 0, Choice, CHOICES(kLfoShapes)},
    {43, "pitchLFOOsc12Rate", "OSC 1&2 LFO Rate", "PLFO Rt", "Hz", 0.05f, 20, 2, Log},
    {44, "pitchLFOOsc12Randomize", "OSC 1&2 LFO Random", "PLFO Rnd", "", 0, 1, 0.3f, Lin},
    {45, "pitchLFOOsc12Enabled", "OSC 1&2 LFO", "PLFO", "", 0, 1, 0, Bool},
    {46, "pitchLFOOsc12Depth", "OSC 1&2 LFO Depth", "PLFO Dp", "", 0, 1, 0.2f, Lin},

    // LFO: pitch, OSC 3
    {47, "pitchLFOOsc3Shape", "OSC 3 LFO Shape", "3LFO Shp", "", 0, 3, 0, Choice, CHOICES(kLfoShapes)},
    {48, "pitchLFOOsc3Rate", "OSC 3 LFO Rate", "3LFO Rt", "Hz", 0.05f, 20, 2, Log},
    {49, "pitchLFOOsc3Randomize", "OSC 3 LFO Random", "3LFO Rnd", "", 0, 1, 0.3f, Lin},
    {50, "pitchLFOOsc3Enabled", "OSC 3 LFO", "3LFO", "", 0, 1, 0, Bool},
    {51, "pitchLFOOsc3Depth", "OSC 3 LFO Depth", "3LFO Dp", "", 0, 1, 0.2f, Lin},

    // Vintage delay
    {67, "delayMasterTime", "Delay Time", "Dly Time", "ms", 10, 2000, 500, Log},
    {68, "delaySubdivide", "Delay Subdivide", "Dly Sub", "", 1, 16, 4, Int},
    {69, "delayTimeLeft", "Delay Time L", "Dly L", "", 1, 16, 3, Int},
    {70, "delayTimeRight", "Delay Time R", "Dly R", "", 1, 16, 5, Int},
    {71, "delayRepeatFeedback", "Delay Feedback", "Dly Fb", "", 0, 1, 0.5f, Lin},
    {72, "delayWet", "Delay Amount", "Dly Amt", "", 0, 4, 0, Lin},
    {73, "delayDry", "Delay Dry", "Dly Dry", "", 0, 4, 1, Lin},
    {74, "delayWidth", "Delay Stereo Width", "Dly Wid", "", -1, 1, 1, Lin},
    {75, "delayMixMode", "Delay Mix Mode", "Dly Mode", "", 0, 3, 1, Choice, CHOICES(kDelayMixModes)},
    {76, "delayMedium", "Delay Medium", "Dly Med", "", 0, 2, 1, Choice, CHOICES(kDelayMedia)},

    // Reverb
    {57, "reverbType", "Reverb Type", "Rev Type", "", 0, 3, 1, Choice, CHOICES(kReverbTypes)},
    {62, "reverbSize", "Reverb Size", "Rev Size", "", 0, 1, 0.5f, Lin},
    {63, "reverbPredelay", "Reverb Predelay", "Rev Pre", "", 0, 1, 0.5f, Lin},
    {64, "reverbMix", "Reverb Mix", "Rev Mix", "", 0, 0.9f, 0, Lin},
    {65, "reverbBodyLevel", "Reverb Body", "Rev Body", "dB", -15, 15, 0, Lin},
    {66, "reverbShimmerLevel", "Reverb Air", "Rev Air", "dB", -15, 15, 0, Lin},

    // Tuning
    {87, "masterTune", "Master Tune", "Tune", "ct", -100, 100, 0, Lin},

    // Derived from the reverb's type and size (see DroneSynthPlugin::updateReverbDerived).
    {58, "reverbLowCutoff", "Reverb Low Cut", "Rev LoCut", "Hz", 50, 1000, 200, Log, nullptr, 0, false},
    {59, "reverbDamping", "Reverb Damping", "Rev Damp", "Hz", 1500, 20000, 16000, Log, nullptr, 0, false},
    {60, "reverbLowDecay", "Reverb Low Decay", "Rev LoDec", "", 1, 8, kHallLowDecay, Lin, nullptr, 0, false},
    {61, "reverbHighDecay", "Reverb High Decay", "Rev HiDec", "", 1, 8, kHallHighDecay, Lin, nullptr, 0, false},
};

#undef CHOICES

constexpr int kCount = int(sizeof(kTable) / sizeof(kTable[0]));

int countExposed() {
    int n = 0;
    while (n < kCount && kTable[n].exposed) ++n;
    return n;
}

} // namespace

const ParamInfo *parameterTable() { return kTable; }
int parameterCount() { return kCount; }

int exposedParameterCount() {
    static const int n = countExposed();
    return n;
}

const ParamInfo *findParameter(int id) {
    for (const auto &p : kTable) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

} // namespace dronesynth
