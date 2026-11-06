//
// Everything the desktop plugin does apart from talking to a plugin format: holds
// the parameter values, turns host MIDI into notes, pitch bend, mod wheel and
// controller (NRPN) moves, and drives the shared engine. Kept free of DPF so it
// can be tested on its own (tests/core_test.cpp).
//

#ifndef DRONESYNTH_PLUGIN_CORE_H
#define DRONESYNTH_PLUGIN_CORE_H

#include <array>
#include <atomic>
#include <cstdint>

#include "NrpnDecoder.h"
#include "ParameterTable.h"
#include "synth/SynthEngine.h"

namespace dronesynth {

struct MidiMessage {
    /// Frame within the block it lands on.
    uint32_t frame;
    uint8_t size;
    uint8_t data[3];
};

class DroneSynthCore {
public:
    DroneSynthCore();

    /// Before the first process() and on every sample-rate change; not concurrently with process().
    void prepare(double sampleRate);

    /// Value by table index (see parameterTable()). Any thread.
    float value(int index) const;
    /// A host's set (automation, its generic editor, a restored project), by table index.
    void setValue(int index, float value);

    /// Renders one block. `events` are sorted by frame; each takes effect on its frame.
    void process(float *left, float *right, int frames, const MidiMessage *events, int eventCount);

    /// Whether a controller moved this parameter (table index) since last asked,
    /// so the format layer can tell the host. Audio thread.
    bool takeControllerChange(int index);

private:
    void set(int id, float value);
    /// The app gives the reverb's crossover, damping and decays no direct control: they
    /// follow the type, and the decays the size too (SynthViewModel.kt).
    void updateReverbDerived(bool typeChanged);
    void handleMidi(const MidiMessage &m);

    synth::SynthEngine engine_;
    NrpnDecoder nrpn_;
    std::array<std::atomic<float>, kMaxParamId> values_;
    std::array<bool, kMaxParamId> controllerChanged_{};
    std::array<int, kMaxParamId> indexById_{};
};

} // namespace dronesynth

#endif
