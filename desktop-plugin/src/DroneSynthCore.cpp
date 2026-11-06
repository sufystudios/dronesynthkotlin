#include "DroneSynthCore.h"

#include <algorithm>
#include <cmath>

#include "synth/Params.h"

namespace dronesynth {

namespace {

/// ReverbKind in SynthParam.kt: release-time scale, crossover Hz, damping Hz.
struct ReverbKindInfo {
    float releaseScale;
    float crossover;
    float damping;
};
constexpr ReverbKindInfo kReverbKinds[4] = {
    {0.4f, 200.0f, 10000.0f},  // Room
    {1.0f, 200.0f, 16000.0f},  // Hall
    {0.6f, 50.0f, 20000.0f},   // Plate
    {1.6f, 400.0f, 14000.0f},  // Cathedral
};

} // namespace

DroneSynthCore::DroneSynthCore() {
    indexById_.fill(-1);
    for (auto &v : values_) v.store(0.0f, std::memory_order_relaxed);
    const ParamInfo *table = parameterTable();
    for (int i = 0; i < parameterCount(); ++i) {
        if (table[i].id < kMaxParamId) indexById_[table[i].id] = i;
        set(table[i].id, table[i].def);
    }
    updateReverbDerived(true);
}

void DroneSynthCore::prepare(double sampleRate) {
    engine_.prepareToPlay(sampleRate);
}

float DroneSynthCore::value(int index) const {
    if (index < 0 || index >= parameterCount()) return 0.0f;
    return values_[parameterTable()[index].id].load(std::memory_order_relaxed);
}

void DroneSynthCore::setValue(int index, float value) {
    if (index < 0 || index >= parameterCount()) return;
    set(parameterTable()[index].id, value);
}

void DroneSynthCore::set(int id, float value) {
    if (id < 0 || id >= kMaxParamId || indexById_[id] < 0 || !std::isfinite(value)) return;
    const ParamInfo &info = parameterTable()[indexById_[id]];
    value = std::clamp(value, info.min, info.max);
    if (info.kind != ParamKind::Linear && info.kind != ParamKind::Log) value = std::round(value);
    const float previous = values_[id].exchange(value, std::memory_order_relaxed);
    engine_.setParam(id, value);
    if (previous == value) return;
    if (id == synth::kReverbType) updateReverbDerived(true);
    if (id == synth::kReverbSize) updateReverbDerived(false);
}

void DroneSynthCore::updateReverbDerived(bool typeChanged) {
    const int type = std::clamp(int(values_[synth::kReverbType].load(std::memory_order_relaxed)), 0, 3);
    const ReverbKindInfo &kind = kReverbKinds[type];
    const float size = values_[synth::kReverbSize].load(std::memory_order_relaxed);
    if (typeChanged) {
        set(synth::kReverbLowCutoff, kind.crossover);
        set(synth::kReverbDamping, kind.damping);
    }
    set(synth::kReverbLowDecay, std::clamp(kind.releaseScale * std::pow(8.0f, size), 1.0f, 8.0f));
    set(synth::kReverbHighDecay, std::clamp(kind.releaseScale * std::pow(7.0f, size), 1.0f, 8.0f));
}

bool DroneSynthCore::takeControllerChange(int index) {
    if (index < 0 || index >= parameterCount()) return false;
    const int id = parameterTable()[index].id;
    const bool changed = controllerChanged_[id];
    controllerChanged_[id] = false;
    return changed;
}

void DroneSynthCore::handleMidi(const MidiMessage &m) {
    if (m.size < 2) return;
    const uint8_t type = m.data[0] & 0xF0;
    const uint8_t channel = m.data[0] & 0x0F;
    const uint8_t d1 = m.data[1] & 0x7F;
    const uint8_t d2 = m.size > 2 ? (m.data[2] & 0x7F) : 0;
    switch (type) {
        case 0x80:
            engine_.noteOff(d1);
            break;
        case 0x90:
            if (d2 == 0) engine_.noteOff(d1); else engine_.noteOn(d1, d2);
            break;
        case 0xB0: {
            const NrpnDecoder::Result r = nrpn_.receive(d1, d2, channel);
            if (r.type == NrpnDecoder::Result::SetParameter) {
                set(r.id, r.value);
                controllerChanged_[r.id] = true;
            } else if (r.type == NrpnDecoder::Result::AllNotesOff) {
                engine_.allSoundOff();
            } else if (d1 == 1) {
                engine_.setParam(synth::kModWheel, d2 / 127.0f);
            } else if (d1 == 120 || d1 == 123) {
                engine_.allSoundOff();
            }
            break;
        }
        case 0xE0:
            engine_.setParam(synth::kPitchBend, float(((d2 << 7) | d1) - 8192) / 8192.0f);
            break;
        default:
            break;
    }
}

void DroneSynthCore::process(float *left, float *right, int frames, const MidiMessage *events, int eventCount) {
    // Render up to each event's frame, then apply it, so notes land where the host put them.
    int done = 0;
    int next = 0;
    while (done < frames) {
        while (next < eventCount && int(events[next].frame) <= done) handleMidi(events[next++]);
        const int until = next < eventCount ? std::min(frames, int(events[next].frame)) : frames;
        engine_.process(left + done, right + done, until - done);
        done = until;
    }
    while (next < eventCount) handleMidi(events[next++]);
}

} // namespace dronesynth
