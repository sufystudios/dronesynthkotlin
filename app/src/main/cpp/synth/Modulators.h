//
// Small modulation sources used by the engine: a fast RNG safe for the audio
// thread, the drifting LFO, and a pink-noise generator.
//

#ifndef DRONESYNTH_MODULATORS_H
#define DRONESYNTH_MODULATORS_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace synth {

/// xorshift32 - deterministic, allocation-free and lock-free, unlike rand().
class FastRandom {
public:
    explicit FastRandom(uint32_t seed = 0x9E3779B9u) : state_(seed ? seed : 1u) {}

    /// Uniform in [0, 1).
    float next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return float(state_ >> 8) * (1.0f / 16777216.0f);
    }

    /// Uniform in [-1, 1).
    float nextBipolar() { return next() * 2.0f - 1.0f; }

private:
    uint32_t state_;
};

/// Shapes, in the same order as the iOS LFOShape enum.
enum class LfoShape : int { Sine = 0, Triangle = 1, Square = 2, Random = 3 };

/// The iOS app's LFOEngine: a free-running LFO whose rate and amplitude wander
/// slightly (scaled by `randomize`) so it never repeats with mechanical perfection.
class DriftingLfo {
public:
    explicit DriftingLfo(uint32_t seed) : random_(seed) {}

    LfoShape shape = LfoShape::Sine;
    float rate = 2.0f;       // Hz
    float randomize = 0.3f;  // 0...1

    /// Advances by `dt` seconds and returns the current value, roughly -1...1.
    float tick(float dt) {
        if (random_.next() < dt / 0.5f) wanderTarget_ = random_.nextBipolar();
        wander_ += (wanderTarget_ - wander_) * 0.05f;

        const float rateJitter = 1.0f + wander_ * 0.4f * randomize;
        const float effectiveRate = std::max(0.01f, rate * rateJitter);
        phase_ += double(effectiveRate) * dt;
        if (phase_ > 1.0e6) phase_ = std::fmod(phase_, 1.0);

        const float ampJitter = 1.0f + wander_ * 0.15f * randomize;
        return waveValue() * ampJitter;
    }

private:
    float waveValue() {
        const double t = phase_ - std::floor(phase_);
        switch (shape) {
            case LfoShape::Sine:
                return float(std::sin(phase_ * 2.0 * M_PI));
            case LfoShape::Triangle:
                return float(t < 0.5 ? 4.0 * t - 1.0 : 3.0 - 4.0 * t);
            case LfoShape::Square:
                return t < 0.5 ? 1.0f : -1.0f;
            case LfoShape::Random: {
                const auto cycle = int64_t(phase_);
                if (cycle != randomCycle_) {
                    randomCycle_ = cycle;
                    randomHold_ = random_.nextBipolar();
                }
                return randomHold_;
            }
        }
        return 0.0f;
    }

    FastRandom random_;
    double phase_ = 0.0;
    float wander_ = 0.0f;
    float wanderTarget_ = 0.0f;
    int64_t randomCycle_ = -1;
    float randomHold_ = 0.0f;
};

/// Pink noise via Paul Kellet's economy filter (-3 dB/octave, within 0.5 dB).
class PinkNoise {
public:
    explicit PinkNoise(uint32_t seed) : random_(seed) {}

    float next() {
        const float white = random_.nextBipolar();
        b0_ = 0.99765f * b0_ + white * 0.0990460f;
        b1_ = 0.96300f * b1_ + white * 0.2965164f;
        b2_ = 0.57000f * b2_ + white * 1.0526913f;
        return (b0_ + b1_ + b2_ + white * 0.1848f) * 0.25f;
    }

private:
    FastRandom random_;
    float b0_ = 0.0f, b1_ = 0.0f, b2_ = 0.0f;
};

/// One-pole smoother for control values, so knob moves never zipper or click.
class Smoothed {
public:
    void setTimeConstant(float seconds, float sampleRate) {
        coeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, seconds * sampleRate));
        advanceFrames_ = -1;
    }
    void snap(float v) { value_ = target_ = v; }
    void setTarget(float v) { target_ = v; }
    float next() {
        value_ += coeff_ * (target_ - value_);
        return value_;
    }
    /// Advances `n` samples at once (exact for a one-pole).
    float advance(int n) {
        // Almost always called with the same n (the control block), so keep the pow().
        if (n != advanceFrames_) {
            advanceFrames_ = n;
            advanceRemaining_ = std::pow(1.0f - coeff_, float(n));
        }
        value_ = target_ + (value_ - target_) * advanceRemaining_;
        return value_;
    }
    float value() const { return value_; }
    float target() const { return target_; }

private:
    float value_ = 0.0f;
    float target_ = 0.0f;
    float coeff_ = 1.0f;
    int advanceFrames_ = -1;
    float advanceRemaining_ = 0.0f;
};

} // namespace synth

#endif //DRONESYNTH_MODULATORS_H
