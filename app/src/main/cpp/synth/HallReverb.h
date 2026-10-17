//
// A hall reverb in the style of Dragonfly Hall: early reflections from a tapped
// delay, feeding a modulated 8-line feedback delay network with input diffusion,
// two-band decay and high-frequency damping. Original code (Dragonfly itself is
// GPL). Its Room, Hall and Cathedral shapes come from the reverb type; Plate has
// its own algorithm (PlateReverb).
//

#ifndef DRONESYNTH_HALLREVERB_H
#define DRONESYNTH_HALLREVERB_H

#include <array>

#include "ReverbBlocks.h"
#include "ReverbSettings.h"

namespace synth {

class HallReverb {
public:
    /// Allocates the delay lines. Not real-time safe.
    void prepare(double sampleRate);
    /// Takes new settings; they glide in over the following blocks.
    void set(const ReverbSettings &settings) { target_ = settings; }
    /// Writes the wet signal only (no dry, no EQ) for `frames` samples.
    void process(const float *inLeft, const float *inRight, float *wetLeft, float *wetRight, int frames);

private:
    static constexpr int kLines = 8;
    static constexpr int kTaps = 12;
    static constexpr int kDiffusers = 4;
    static constexpr int kSubBlock = 32;

    struct Shape {
        float scale;      // room dimensions relative to the Hall
        float early;      // early reflection level
        float late;       // late reverb level
        float diffusion;  // 0...1
        float modDepthMs; // delay-line modulation ("wander")
        float modRateHz;  // ("spin")
        float width;      // stereo width of the wet signal
    };
    static const Shape kShapes[4];

    void updateCoefficients();

    double sampleRate_ = 48000.0;
    float rateScale_ = 1.0f;  // sample rate relative to 48 kHz
    ReverbSettings target_{};
    ReverbSettings applied_{};
    bool primed_ = false;

    // Smoothed controls.
    float dimension_ = 1.0f;
    float predelay_ = 0.0f;  // samples
    float early_ = 0.0f, late_ = 0.0f, diffusion_ = 0.0f, width_ = 1.0f;
    float modDepth_ = 0.0f, modRate_ = 0.0f;
    float smooth_ = 0.0f;  // per-sub-block smoothing coefficient

    // Derived per sub-block.
    std::array<float, kLines> lineLength_{};
    std::array<float, kLines> gainMid_{};
    std::array<float, kLines> gainLow_{};
    float crossoverCoeff_ = 0.0f;
    float dampingCoeff_ = 0.0f;

    reverb::Delay inputLeft_, inputRight_;
    std::array<float, kTaps> tapGainLeft_{}, tapGainRight_{};
    std::array<reverb::Allpass, 2> earlySmearLeft_, earlySmearRight_;
    std::array<reverb::Allpass, kDiffusers> diffusers_;
    std::array<reverb::Delay, kLines> lines_;
    std::array<reverb::Allpass, kLines> lineAllpass_;
    std::array<reverb::DecayStage, kLines> decay_;
    std::array<float, kLines> lineOut_{};
    std::array<double, kLines> lfoPhase_{};
};

} // namespace synth

#endif //DRONESYNTH_HALLREVERB_H
