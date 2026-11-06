//
// The DPF wrapper: hands host parameters, audio and MIDI to DroneSynthCore.
// DPF builds this one source into VST3, CLAP, AU and a standalone app.
//

#include "DistrhoPlugin.hpp"

#include "DroneSynthCore.h"

START_NAMESPACE_DISTRHO

using dronesynth::DroneSynthCore;
using dronesynth::MidiMessage;
using dronesynth::ParamInfo;
using dronesynth::ParamKind;

class DroneSynthPlugin : public Plugin {
public:
    DroneSynthPlugin()
        : Plugin(uint32_t(dronesynth::exposedParameterCount()), 0, 0) {
        core_.prepare(getSampleRate());
    }

protected:
    // --- information

    const char *getLabel() const override { return "DroneSynth"; }
    const char *getDescription() const override {
        return "A three-oscillator drone synth with stereo Moog filters, a vintage delay and reverb. "
               "Follows the Drone Synth phone app's Motion Controller.";
    }
    const char *getMaker() const override { return "Sufy Studios"; }
    const char *getHomePage() const override { return "https://sufystudios.xyz"; }
    const char *getLicense() const override { return "Proprietary"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }
    int64_t getUniqueId() const override { return d_cconst('D', 'r', 'S', 'y'); }

    // --- parameters

    void initParameter(uint32_t index, Parameter &parameter) override {
        const ParamInfo &info = dronesynth::parameterTable()[index];
        parameter.hints = kParameterIsAutomatable;
        parameter.name = info.name;
        parameter.shortName = info.shortName;
        parameter.symbol = info.symbol;
        parameter.unit = info.unit;
        parameter.ranges.min = info.min;
        parameter.ranges.max = info.max;
        parameter.ranges.def = info.def;
        switch (info.kind) {
            case ParamKind::Linear:
                break;
            case ParamKind::Log:
                parameter.hints |= kParameterIsLogarithmic;
                break;
            case ParamKind::Integer:
                parameter.hints |= kParameterIsInteger;
                break;
            case ParamKind::Boolean:
                parameter.hints |= kParameterIsBoolean | kParameterIsInteger;
                break;
            case ParamKind::Choice: {
                parameter.hints |= kParameterIsInteger;
                parameter.enumValues.count = uint8_t(info.labelCount);
                parameter.enumValues.restrictedMode = true;
                auto *values = new ParameterEnumerationValue[info.labelCount];
                for (int i = 0; i < info.labelCount; ++i) {
                    values[i].value = float(i);
                    values[i].label = info.labels[i];
                }
                parameter.enumValues.values = values;
                break;
            }
        }
    }

    float getParameterValue(uint32_t index) const override { return core_.value(int(index)); }

    void setParameterValue(uint32_t index, float value) override { core_.setValue(int(index), value); }

    // --- processing

    void activate() override { core_.prepare(getSampleRate()); }

    void sampleRateChanged(double newSampleRate) override { core_.prepare(newSampleRate); }

    void run(const float **, float **outputs, uint32_t frames, const MidiEvent *midiEvents,
             uint32_t midiEventCount) override {
        uint32_t count = 0;
        for (uint32_t i = 0; i < midiEventCount && count < kMaxEvents; ++i) {
            const MidiEvent &e = midiEvents[i];
            if (e.size == 0 || e.size > 3) continue;  // SysEx and the like
            MidiMessage &m = events_[count++];
            m.frame = e.frame;
            m.size = uint8_t(e.size);
            for (uint32_t b = 0; b < e.size; ++b) m.data[b] = e.data[b];
        }
        core_.process(outputs[0], outputs[1], int(frames), events_, int(count));
    }

private:
    static constexpr uint32_t kMaxEvents = 1024;

    DroneSynthCore core_;
    MidiMessage events_[kMaxEvents];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DroneSynthPlugin)
};

Plugin *createPlugin() { return new DroneSynthPlugin(); }

END_NAMESPACE_DISTRHO
