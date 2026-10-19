//
// Stereo tape-style delay modelled on the behaviour of Calf Studio Gear's Vintage
// Delay. Ported from the iOS app's VintageDelayDSP.mm (same DSP, with the AudioKit
// DSPBase/ParameterRamper plumbing replaced by plain buffers and Smoothed values).
// See VintageDelay.cpp for the signal flow.
//

#ifndef DRONESYNTH_VINTAGEDELAY_H
#define DRONESYNTH_VINTAGEDELAY_H

#include <cstdint>
#include <vector>
#include "Modulators.h"

namespace synth {

/// Length of each channel's delay line, in samples (a power of two, so the ring
/// buffer can wrap with a mask). About 10.9 s at 48 kHz. Settings that would need
/// a longer line are clamped rather than wrapped.
constexpr int32_t kVintageDelayBufferLength = 524288;

/// Mix modes, in Calf's order. Values are stored in presets - never reorder.
enum VintageDelayMixMode : int32_t {
    kMixModeStereo = 0,
    kMixModePingPong = 1,
    kMixModeLeftThenRight = 2,
    kMixModeRightThenLeft = 3,
};

/// Media, in Calf's order. Values are stored in presets - never reorder.
enum VintageDelayMedium : int32_t {
    kMediumPlain = 0,
    kMediumTape = 1,
    kMediumOldTape = 2,
};

/// Resolved delay lengths for one set of timing controls.
struct VintageDelayTaps {
    int32_t left;   // samples
    int32_t right;  // samples
    bool clamped;   // requested lengths didn't fit the line and were shortened
};

/// The single source of truth for delay timing, used by the kernel and (via JNI)
/// by the UI readout, so what's displayed is what's heard.
VintageDelayTaps vintageDelayComputeTaps(float timeMs, float subdivide, float timeLeft, float timeRight,
                                         int32_t mixMode, double sampleRate);

class VintageDelay {
public:
    void init(double sampleRate);
    void reset();

    // Control values; continuous ones are smoothed per sample, stepped ones
    // (timing, mix mode, medium) switch via a short crossfade.
    float time = 500.0f;
    float subdivide = 4.0f;
    float timeLeft = 3.0f;
    float timeRight = 5.0f;
    int32_t mixMode = kMixModePingPong;
    int32_t medium = kMediumTape;
    void setFeedback(float v) { feedback_.setTarget(v); }
    void setAmount(float v) { amount_.setTarget(v); }
    void setDry(float v) { dry_.setTarget(v); }
    void setWidth(float v) { width_.setTarget(v); }
    void snapSmoothedValues();

    /// In-place stereo processing. At a wet amount of 0 the delay is bypassed.
    void process(float *left, float *right, int frames);

private:
    struct Biquad {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        void setLowPass(double frequency, double q, double sampleRate);
        void setBandPass(double frequency, double q, double sampleRate);
        float process(float input);
        void clear() { x1 = x2 = y1 = y2 = 0; }
    };

    struct TapSet {
        int32_t outLeft = 1, outRight = 1;
        int32_t feedbackLeft = 1, feedbackRight = 1;
        bool crossFeedback = false;
        float feedbackExponentLeft = 1, feedbackExponentRight = 1;
        float amountExponentLeft = 0, amountExponentRight = 0;
        bool operator==(const TapSet &o) const;
        bool operator!=(const TapSet &o) const { return !(*this == o); }
    };

    struct Frame { float wetLeft, wetRight, feedbackLeft, feedbackRight; };

    void clearState();
    void bypass(float *left, float *right, int frames);
    TapSet makeTaps() const;
    Frame evaluate(const TapSet &set, float feedback, float amount) const;
    float read(const std::vector<float> &line, int32_t distance) const {
        return line[(writeIndex_ - uint32_t(distance)) & kMask];
    }

    static constexpr uint32_t kMask = kVintageDelayBufferLength - 1;
    static constexpr double kTapCrossfadeSeconds = 0.025;
    static constexpr double kMediumGlideSeconds = 0.02;
    // Zeroing both 2 MB lines at once could make a slow phone miss a callback,
    // so a bypassed delay clears them this many samples per block.
    static constexpr uint32_t kClearStep = 16384;

    double sampleRate_ = 48000.0;
    Smoothed feedback_, amount_, dry_, width_;
    std::vector<float> lineLeft_, lineRight_;
    uint32_t writeIndex_ = 0;
    TapSet currentTaps_, nextTaps_;
    bool hasTaps_ = false;
    bool bypassed_ = false;
    uint32_t clearedUpTo_ = 0;  // while bypassed, the lines are zero below this index
    bool crossfading_ = false;
    float crossfadePosition_ = 0, crossfadeStep_ = 1;
    Biquad bandPassLeft_, bandPassRight_, lowPassLeft_, lowPassRight_;
    float mediumWeights_[3] = {0, 1, 0};
    float mediumGlide_ = 1;
};

} // namespace synth

#endif //DRONESYNTH_VINTAGEDELAY_H
