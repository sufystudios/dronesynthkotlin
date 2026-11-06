// Offline checks for the desktop plugin's core: run `dronesynth_tests`.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "DroneSynthCore.h"
#include "synth/Params.h"

using namespace dronesynth;

static int failures = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            ++failures;                                   \
            std::printf("FAIL %s:%d: ", __FILE__, __LINE__); \
            std::printf(__VA_ARGS__);                     \
            std::printf("\n");                            \
        }                                                 \
    } while (0)

static int indexOf(int id) {
    for (int i = 0; i < parameterCount(); ++i)
        if (parameterTable()[i].id == id) return i;
    return -1;
}

static float rms(const std::vector<float> &v) {
    double sum = 0;
    for (float x : v) sum += double(x) * x;
    return float(std::sqrt(sum / v.size()));
}

/// Renders `blocks` blocks of 256 frames into left/right.
static void render(DroneSynthCore &core, std::vector<float> &l, std::vector<float> &r, int blocks,
                   const std::vector<MidiMessage> &firstBlockEvents = {}) {
    l.assign(256 * blocks, 0.0f);
    r.assign(256 * blocks, 0.0f);
    for (int b = 0; b < blocks; ++b) {
        const auto &ev = b == 0 ? firstBlockEvents : std::vector<MidiMessage>{};
        core.process(l.data() + 256 * b, r.data() + 256 * b, 256, ev.data(), int(ev.size()));
    }
}

static std::vector<MidiMessage> nrpn(int id, float value, uint32_t frame = 0) {
    std::vector<MidiMessage> out;
    for (const auto &cc : NrpnDecoder::encode(id, value)) out.push_back({frame, 3, {cc[0], cc[1], cc[2]}});
    return out;
}

static void testTable() {
    const ParamInfo *t = parameterTable();
    for (int i = 0; i < parameterCount(); ++i) {
        CHECK(t[i].min < t[i].max, "%s range", t[i].symbol);
        CHECK(t[i].def >= t[i].min && t[i].def <= t[i].max, "%s default out of range", t[i].symbol);
        CHECK(t[i].kind != ParamKind::Choice || t[i].labelCount == int(t[i].max - t[i].min) + 1,
              "%s labels", t[i].symbol);
        for (int j = i + 1; j < parameterCount(); ++j) CHECK(t[i].id != t[j].id, "duplicate id %d", t[i].id);
        if (i >= exposedParameterCount()) CHECK(!t[i].exposed, "%s: hidden entries must come last", t[i].symbol);
    }
    CHECK(exposedParameterCount() == parameterCount() - 4, "exposed count %d", exposedParameterCount());
}

static void testNrpnRoundTrip() {
    // Every parameter survives the phone's encoding to within one 14-bit step.
    for (int i = 0; i < parameterCount(); ++i) {
        const ParamInfo &p = parameterTable()[i];
        for (float pos : {0.0f, 0.37f, 1.0f}) {
            float v = p.min + pos * (p.max - p.min);
            if (p.kind == ParamKind::Choice || p.kind == ParamKind::Boolean) v = std::round(v);
            NrpnDecoder d;
            NrpnDecoder::Result r;
            for (const auto &cc : NrpnDecoder::encode(p.id, v)) r = d.receive(cc[1], cc[2], 0);
            CHECK(r.type == NrpnDecoder::Result::SetParameter && r.id == p.id, "%s not decoded", p.symbol);
            CHECK(std::fabs(r.value - v) <= (p.max - p.min) / 16383.0f + 1e-4f, "%s: sent %f got %f", p.symbol, v,
                  r.value);
        }
    }
    // A known iOS message: cutoff (NRPN 0) at full scale = 20 kHz.
    NrpnDecoder d;
    d.receive(99, 0, 0);
    d.receive(98, 0, 0);
    d.receive(6, 127, 0);
    auto r = d.receive(38, 127, 0);
    CHECK(r.type == NrpnDecoder::Result::SetParameter && r.id == 0 && r.value == 20000.0f, "cutoff max");
    // An RPN in between cancels the selection.
    d.receive(99, 0, 0);
    d.receive(98, 0, 0);
    d.receive(101, 0, 0);
    d.receive(6, 2, 0);
    r = d.receive(38, 0, 0);
    CHECK(r.type == NrpnDecoder::Result::None, "RPN data must not set a parameter");
    // The silence command.
    for (const auto &cc : NrpnDecoder::encode(kAllNotesOffNrpn, 0)) r = d.receive(cc[1], cc[2], 0);
    CHECK(r.type == NrpnDecoder::Result::AllNotesOff, "NRPN 16000 = all notes off");
    // iOS's ARP/SEQ addresses (77...86) aren't parameters here yet: ignored.
    for (const auto &cc : NrpnDecoder::encode(80, 0)) r = d.receive(cc[1], cc[2], 0);
    CHECK(r.type == NrpnDecoder::Result::None, "sequencer NRPN ignored");
}

static void testDefaultsAndReverb() {
    DroneSynthCore core;
    CHECK(std::fabs(core.value(indexOf(synth::kCutoff)) - 894.4f) < 1.0f, "cutoff default");
    CHECK(core.value(indexOf(synth::kOsc1Waveform)) == 3.0f, "osc1 saw");
    // Hall at size 0.5: low decay sqrt(8).
    CHECK(std::fabs(core.value(indexOf(synth::kReverbLowDecay)) - std::sqrt(8.0f)) < 1e-4f, "hall low decay");
    core.setValue(indexOf(synth::kReverbType), 3);  // Cathedral
    CHECK(core.value(indexOf(synth::kReverbLowCutoff)) == 400.0f, "cathedral crossover");
    CHECK(core.value(indexOf(synth::kReverbDamping)) == 14000.0f, "cathedral damping");
    CHECK(std::fabs(core.value(indexOf(synth::kReverbLowDecay)) - std::min(8.0f, 1.6f * std::sqrt(8.0f))) < 1e-4f,
          "cathedral low decay");
    core.setValue(indexOf(synth::kReverbSize), 0.0f);
    CHECK(std::fabs(core.value(indexOf(synth::kReverbHighDecay)) - 1.6f) < 1e-4f, "size 0 high decay");
    // Hosts' in-between values snap for stepped parameters, and clamp.
    core.setValue(indexOf(synth::kOsc1Waveform + synth::kOscOctaveOffset), 1.4f);
    CHECK(core.value(indexOf(synth::kOsc1Waveform + synth::kOscOctaveOffset)) == 1.0f, "octave rounds");
    core.setValue(indexOf(synth::kCutoff), 99999.0f);
    CHECK(core.value(indexOf(synth::kCutoff)) == 20000.0f, "cutoff clamps");
}

static void testRendering() {
    DroneSynthCore core;
    core.prepare(48000.0);
    std::vector<float> l, r;

    render(core, l, r, 20);
    CHECK(rms(l) == 0.0f, "silent before a note");

    // A note on halfway through the first block: nothing before its frame.
    render(core, l, r, 40, {{128, 3, {0x90, 57, 100}}});
    float before = 0;
    for (int i = 0; i < 128; ++i) before = std::max(before, std::fabs(l[i]));
    CHECK(before == 0.0f, "sound before the note's frame (%f)", before);
    CHECK(rms(l) > 0.01f, "no sound after note on (rms %f)", rms(l));
    for (float x : l) CHECK(std::isfinite(x) && std::fabs(x) <= 1.0f, "sample out of range %f", x);

    // The controller turns the cutoff down: darker, and the change is flagged for the host.
    const int cutoff = indexOf(synth::kCutoff);
    core.takeControllerChange(cutoff);
    render(core, l, r, 40, nrpn(synth::kCutoff, 40.0f));
    CHECK(core.value(cutoff) < 41.0f, "controller cutoff not applied (%f)", core.value(cutoff));
    CHECK(core.takeControllerChange(cutoff), "controller change not flagged");
    CHECK(!core.takeControllerChange(cutoff), "flag not cleared");
    const float dark = rms(std::vector<float>(l.end() - 2048, l.end()));

    core.setValue(cutoff, 20000.0f);
    render(core, l, r, 40);
    const float bright = rms(std::vector<float>(l.end() - 2048, l.end()));
    CHECK(bright > dark * 1.5f, "cutoff makes no difference (dark %f, bright %f)", dark, bright);
    CHECK(!core.takeControllerChange(cutoff), "host sets aren't controller changes");

    // Note off with a short release, then silence.
    core.setValue(indexOf(synth::kRelease), 0.002f);
    render(core, l, r, 100, {{0, 3, {0x80, 57, 0}}});
    CHECK(rms(std::vector<float>(l.end() - 2048, l.end())) < 1e-4f, "still sounding after note off");

    // The phone's silence command stops a drone.
    core.setValue(indexOf(synth::kDroneMode), 1);
    render(core, l, r, 20, {{0, 3, {0x90, 50, 100}}});
    CHECK(rms(l) > 0.01f, "drone silent");
    render(core, l, r, 200, nrpn(kAllNotesOffNrpn, 0));
    CHECK(rms(std::vector<float>(l.end() - 2048, l.end())) < 1e-4f, "drone not silenced by NRPN 16000");

    // A new rate mid-session keeps working.
    core.prepare(96000.0);
    render(core, l, r, 40, {{0, 3, {0x90, 62, 90}}});
    CHECK(rms(l) > 0.01f, "silent after a rate change");
}

int main() {
    testTable();
    testNrpnRoundTrip();
    testDefaultsAndReverb();
    testRendering();
    if (failures) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("All desktop plugin core tests passed (%d parameters, %d exposed)\n", parameterCount(),
                exposedParameterCount());
    return 0;
}
