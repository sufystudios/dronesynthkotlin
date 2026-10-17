//
// One oscillator slot (the iOS OscillatorSlotEngine): the band-limited
// wavetable oscillator from CWTOscillator, plus pink noise, click-free
// waveform switching and hard sync.
//

#ifndef DRONESYNTH_OSCILLATORSLOT_H
#define DRONESYNTH_OSCILLATORSLOT_H

#include <cmath>
#include "../WTOscillator.h"
#include "Modulators.h"

namespace synth {

/// Waveforms, in the same order as the iOS OscillatorWaveform enum.
enum class Waveform : int { Sine = 0, Triangle = 1, Square = 2, Saw = 3, Pink = 4 };

/// Exposes CWTOscillator's read position so another oscillator can hard-sync to it.
class SyncableWTOscillator : public CWTOscillator {
public:
    double readIndex() const { return m_dReadIndex; }
    double readIncrement() const { return m_dWT_inc; }
    void setReadIndex(double index) {
        m_dReadIndex = index;
        checkWrapIndex(m_dReadIndex);
    }
};

class OscillatorSlot {
public:
    void init(double sampleRate, uint32_t seed) {
        osc_.setSampleRate(sampleRate);
        osc_.m_uWaveform = toPirkleWaveform(Waveform::Sine);
        osc_.startOscillator();
        pink_ = PinkNoise(seed);
        switchGain_.setTimeConstant(0.003f, float(sampleRate));
        switchGain_.snap(1.0f);
        volume.setTimeConstant(0.01f, float(sampleRate));
    }

    void setWaveform(Waveform w) {
        if (w == pendingWaveform_) return;
        pendingWaveform_ = w;
        // Fade out, swap at silence (see render), fade back in.
        switchGain_.setTarget(0.0f);
    }

    /// Control rate: applies a new sounding frequency, reselecting the band-limited table.
    void setFrequency(double hz) {
        osc_.m_dOscFo = hz;
        osc_.update();
    }

    /// Renders one sample at unit amplitude (before `volume`).
    float render() {
        if (waveform_ != pendingWaveform_ && switchGain_.value() < 0.001f) {
            waveform_ = pendingWaveform_;
            if (waveform_ != Waveform::Pink) {
                osc_.m_uWaveform = toPirkleWaveform(waveform_);
                osc_.update();
            }
            switchGain_.setTarget(1.0f);
        }
        const double before = osc_.readIndex();
        float out = float(osc_.doOscillate());
        const double after = osc_.readIndex();
        const double inc = osc_.readIncrement();
        wrapped_ = inc > 0.0 && after < before;
        samplesSinceWrap_ = wrapped_ ? after / inc : 0.0;
        if (waveform_ == Waveform::Pink) out = pink_.next() * 2.5f;
        return out * switchGain_.next();
    }

    /// True when the last render() crossed the start of a cycle.
    bool wrapped() const { return wrapped_; }

    /// When wrapped(), how far past the cycle start the last sample landed, in samples.
    double samplesSinceWrap() const { return samplesSinceWrap_; }

    /// Hard sync: restart this oscillator's cycle as if it began `samplesAgo` ago,
    /// keeping the sub-sample phase so the reset doesn't add extra jitter.
    void hardSync(double samplesAgo) {
        osc_.setReadIndex(samplesAgo * osc_.readIncrement());
    }

    Smoothed volume;

private:
    static UINT toPirkleWaveform(Waveform w) {
        switch (w) {
            case Waveform::Triangle: return COscillator::TRI;
            case Waveform::Square: return COscillator::SQUARE;
            case Waveform::Saw: return COscillator::SAW1;
            case Waveform::Sine:
            case Waveform::Pink:
            default: return COscillator::SINE;
        }
    }

    SyncableWTOscillator osc_;
    PinkNoise pink_{1};
    Waveform waveform_ = Waveform::Sine;
    Waveform pendingWaveform_ = Waveform::Sine;
    Smoothed switchGain_;
    bool wrapped_ = false;
    double samplesSinceWrap_ = 0.0;
};

} // namespace synth

#endif //DRONESYNTH_OSCILLATORSLOT_H
