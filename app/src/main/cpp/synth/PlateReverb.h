//
// A plate reverb after Jon Dattorro's "Effect Design, Part 1" (JAES, 1997): four
// input diffusers feeding a figure-eight "tank" of modulated allpasses and delays,
// with the output built from taps spread around the tank. On top of the paper's
// design, the decay is split into low and mid bands with damping above, so the
// Reverb drawer's knobs work as they do for the hall.
//

#ifndef DRONESYNTH_PLATEREVERB_H
#define DRONESYNTH_PLATEREVERB_H

#include <array>

#include "ReverbBlocks.h"
#include "ReverbSettings.h"

namespace synth {

class PlateReverb {
public:
    /// Allocates the delay lines. Not real-time safe.
    void prepare(double sampleRate);
    /// Takes new settings; they glide in over the following blocks.
    void set(const ReverbSettings &settings) { target_ = settings; }
    /// Silences the tail, as if just prepared. Real-time safe (no allocation).
    void clear();
    /// Writes the wet signal only (no dry, no EQ) for `frames` samples.
    void process(const float *inLeft, const float *inRight, float *wetLeft, float *wetRight, int frames);

private:
    static constexpr int kSubBlock = 32;

    /// One half of the tank: modulated allpass -> delay -> decay -> allpass -> delay -> decay.
    struct Half {
        reverb::Allpass modAllpass;
        reverb::Delay delay1;
        reverb::Allpass allpass;
        reverb::Delay delay2;
        reverb::DecayStage decay1, decay2;
        float feedback = 0.0f;  // this half's output, fed to the other half next sample
    };

    void updateCoefficients();

    double sampleRate_ = 48000.0;
    float rateScale_ = 1.0f;  // sample rate relative to the paper's 29.761 kHz
    ReverbSettings target_{};
    ReverbSettings applied_{};
    bool primed_ = false;
    float smooth_ = 0.0f;

    float scale_ = 1.0f;     // tank size (smoothed)
    float predelay_ = 0.0f;  // samples (smoothed)
    float gainLow1_ = 0.0f, gainMid1_ = 0.0f, gainLow2_ = 0.0f, gainMid2_ = 0.0f;
    float decayDiffusion2_ = 0.5f;
    float crossoverCoeff_ = 0.0f, dampingCoeff_ = 0.0f;
    std::array<float, 5> derivedFrom_{};  // the glided values the above were last worked out from
    double lfoPhase_ = 0.0;

    reverb::Delay input_;
    std::array<reverb::Allpass, 4> diffusers_;
    Half left_, right_;
};

} // namespace synth

#endif //DRONESYNTH_PLATEREVERB_H
