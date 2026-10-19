#include "HallReverb.h"

#include <algorithm>
#include <cmath>

namespace synth {

using namespace reverb;

namespace {

/// Feedback delay line lengths at 48 kHz for the Hall at its middle size (35-73 ms),
/// mutually prime so their echoes never line up.
constexpr float kLineBase[8] = {1693, 1951, 2203, 2467, 2731, 2999, 3253, 3517};
/// In-loop allpasses (thicken the tail) and the input diffusers, at 48 kHz.
constexpr int kLineAllpass[8] = {149, 211, 263, 307, 359, 401, 457, 503};
constexpr int kDiffuser[4] = {113, 167, 271, 379};
constexpr int kEarlySmearLeft[2] = {97, 233};
constexpr int kEarlySmearRight[2] = {109, 251};

/// Early reflection taps (ms at the Hall's middle size). Odd taps reflect off the
/// opposite side of the room, so they read the other input channel.
constexpr float kTapMsLeft[12] = {4.3f, 7.9f, 11.2f, 16.8f, 21.5f, 27.3f, 33.9f, 41.2f, 49.7f, 58.1f, 67.4f, 78.9f};
constexpr float kTapMsRight[12] = {5.1f, 9.4f, 12.7f, 15.3f, 23.6f, 29.8f, 36.1f, 44.3f, 52.6f, 61.9f, 71.0f, 83.2f};
constexpr float kTapSignLeft[12] = {1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, -1};
constexpr float kTapSignRight[12] = {1, 1, -1, 1, -1, -1, 1, -1, -1, 1, -1, 1};

/// How the input enters each line and how the lines sum to left and right
/// (orthogonal, so left and right are decorrelated).
constexpr float kInSign[8] = {1, 1, -1, 1, -1, -1, 1, -1};
constexpr float kOutLeft[8] = {1, -1, 1, 1, -1, 1, -1, -1};
constexpr float kOutRight[8] = {1, 1, -1, -1, 1, 1, -1, -1};

/// Sets the wet level to match the Zita reverb this replaced, so presets keep their balance.
constexpr float kWetGain = 0.88f;

constexpr float kMaxDimension = 1.6f * 1.4f;  // Cathedral at full size
constexpr float kMaxModMs = 1.0f;

} // namespace

// Room, Hall, Plate (unused: PlateReverb plays it), Cathedral.
const HallReverb::Shape HallReverb::kShapes[4] = {
    {0.50f, 0.80f, 0.55f, 0.60f, 0.20f, 0.9f, 0.9f},
    {1.00f, 0.50f, 0.85f, 0.75f, 0.45f, 0.6f, 1.0f},
    {1.00f, 0.50f, 0.85f, 0.75f, 0.45f, 0.6f, 1.0f},
    {1.60f, 0.35f, 0.95f, 0.70f, 0.70f, 0.4f, 1.0f},
};

void HallReverb::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    rateScale_ = float(sampleRate / 48000.0);
    const int modMax = int(std::ceil(kMaxModMs * sampleRate / 1000.0)) + 4;

    // Pre-delay (up to 100 ms) plus the latest early reflection at the largest size.
    const int inputLength = int(sampleRate * (0.1 + 0.084 * kMaxDimension)) + 8;
    inputLeft_.allocate(inputLength);
    inputRight_.allocate(inputLength);
    // Later reflections are quieter (travelled further, bounced more).
    for (int t = 0; t < kTaps; ++t) {
        tapGainLeft_[t] = 0.4f * kTapSignLeft[t] * std::exp(-kTapMsLeft[t] / 45.0f);
        tapGainRight_[t] = 0.4f * kTapSignRight[t] * std::exp(-kTapMsRight[t] / 45.0f);
    }
    for (int k = 0; k < 2; ++k) {
        earlySmearLeft_[k].allocate(int(kEarlySmearLeft[k] * rateScale_));
        earlySmearRight_[k].allocate(int(kEarlySmearRight[k] * rateScale_));
    }
    for (int k = 0; k < kDiffusers; ++k) diffusers_[k].allocate(int(kDiffuser[k] * rateScale_));
    for (int i = 0; i < kLines; ++i) {
        lines_[i].allocate(int(kLineBase[i] * rateScale_ * kMaxDimension) + modMax);
        lineAllpass_[i].allocate(int(kLineAllpass[i] * rateScale_));
        decay_[i] = DecayStage{};
        lineOut_[i] = 0.0f;
        lfoPhase_[i] = double(i) / kLines;
    }
    smooth_ = 1.0f - std::exp(-float(kSubBlock) / float(0.1 * sampleRate));
    primed_ = false;
    derivedFrom_.fill(-1.0f);
}

void HallReverb::clear() {
    for (auto *d : {&inputLeft_, &inputRight_}) d->clear();
    for (auto *group : {&earlySmearLeft_, &earlySmearRight_}) for (auto &a : *group) a.clear();
    for (auto &a : diffusers_) a.clear();
    for (int i = 0; i < kLines; ++i) {
        lines_[i].clear();
        lineAllpass_[i].clear();
        decay_[i] = DecayStage{};
        lineOut_[i] = 0.0f;
    }
    primed_ = false;  // the settings may have moved meanwhile: snap to them
}

void HallReverb::updateCoefficients() {
    const Shape &shape = kShapes[std::clamp(int(target_.type), 0, 3)];
    const float dimension = shape.scale * (0.6f + 0.8f * std::clamp(target_.size, 0.0f, 1.0f));
    const float predelay = std::clamp(target_.predelayMs, 0.0f, 100.0f) * float(sampleRate_) / 1000.0f;

    // Everything glides (~100 ms), so turning a knob or switching type never clicks.
    const float k = primed_ ? smooth_ : 1.0f;
    auto glide = [k](float &value, float to) { value += k * (to - value); };
    glide(dimension_, dimension);
    glide(predelay_, predelay);
    glide(early_, shape.early);
    glide(late_, shape.late);
    glide(diffusion_, shape.diffusion);
    glide(width_, shape.width);
    glide(modDepth_, shape.modDepthMs);
    glide(modRate_, shape.modRateHz);
    glide(applied_.rt60Low, std::clamp(target_.rt60Low, 0.1f, 30.0f));
    glide(applied_.rt60Mid, std::clamp(target_.rt60Mid, 0.1f, 30.0f));
    glide(applied_.crossoverHz, target_.crossoverHz);
    glide(applied_.dampingHz, target_.dampingHz);
    primed_ = true;

    // The pow() and exp() below are costly on slow phones; skip them once the glides settle.
    const std::array<float, 5> inputs = {dimension_, applied_.rt60Low, applied_.rt60Mid,
                                         applied_.crossoverHz, applied_.dampingHz};
    if (inputs == derivedFrom_) return;
    derivedFrom_ = inputs;

    const float sr = float(sampleRate_);
    for (int i = 0; i < kLines; ++i) {
        lineLength_[i] = kLineBase[i] * rateScale_ * dimension_;
        // Each pass through a line loses -60 dB × (its length / RT60).
        const float loop = lineLength_[i] + float(lineAllpass_[i].length);
        gainMid_[i] = std::pow(10.0f, -3.0f * loop / (applied_.rt60Mid * sr));
        gainLow_[i] = std::pow(10.0f, -3.0f * loop / (applied_.rt60Low * sr));
    }
    crossoverCoeff_ = onePoleCoeff(applied_.crossoverHz, sampleRate_);
    dampingCoeff_ = onePoleCoeff(applied_.dampingHz, sampleRate_);
}

void HallReverb::process(const float *inLeft, const float *inRight, float *wetLeft, float *wetRight, int frames) {
    const float msToSamples = float(sampleRate_) / 1000.0f;
    // Delay times are ramped a sample at a time: stepping them once per sub-block
    // would jump the read positions and click.
    std::array<float, kLines> lineDelay{}, lineStep{};

    for (int start = 0; start < frames; start += kSubBlock) {
        const int n = std::min(kSubBlock, frames - start);
        const bool wasPrimed = primed_;
        std::array<float, kLines> fromLength = lineLength_;
        float predelay = predelay_, dimension = dimension_;
        updateCoefficients();
        if (!wasPrimed) {
            fromLength = lineLength_;
            predelay = predelay_;
            dimension = dimension_;
        }
        const float predelayStep = (predelay_ - predelay) / float(n);
        const float dimensionStep = (dimension_ - dimension) / float(n);

        // Unless the size or pre-delay is gliding (the usual case), the early taps hold
        // still, so their positions and interpolation are worked out once per sub-block.
        const bool tapsSettled = predelayStep == 0.0f && dimensionStep == 0.0f;
        std::array<Delay::Tap, kTaps> tapLeft, tapRight;
        Delay::Tap lateTap;
        if (tapsSettled) {
            const float tapScale = msToSamples * dimension;
            for (int t = 0; t < kTaps; ++t) {
                tapLeft[t].set(predelay + kTapMsLeft[t] * tapScale);
                tapRight[t].set(predelay + kTapMsRight[t] * tapScale);
            }
            lateTap.set(predelay + 1.0f);
        }

        // Each line's delay also wanders on its own slow sine ("spin").
        const double phaseStep = double(modRate_) * n / sampleRate_;
        for (int i = 0; i < kLines; ++i) {
            const float rate = 1.0f + 0.07f * float(i);
            const float from = fromLength[i] + modDepth_ * msToSamples * std::sin(kTwoPi * float(lfoPhase_[i]));
            lfoPhase_[i] += phaseStep * rate;
            lfoPhase_[i] -= std::floor(lfoPhase_[i]);
            const float to = lineLength_[i] + modDepth_ * msToSamples * std::sin(kTwoPi * float(lfoPhase_[i]));
            lineDelay[i] = from;
            lineStep[i] = (to - from) / float(n);
        }

        const float smearGain = 0.5f * diffusion_;
        const float diffuserGain = 0.5f + 0.25f * diffusion_;
        const float loopAllpassGain = 0.35f * diffusion_;
        const float sideGain = 0.5f * width_;

        for (int j = start; j < start + n; ++j) {
            inputLeft_.push(inLeft[j]);
            inputRight_.push(inRight[j]);
            predelay += predelayStep;
            dimension += dimensionStep;

            // Early reflections: taps on the pre-delayed input, smeared by two allpasses.
            float earlyLeft = 0.0f, earlyRight = 0.0f;
            const float tapScale = msToSamples * dimension;
            for (int t = 0; t < kTaps; ++t) {
                const Delay &fromLeft = (t & 1) ? inputRight_ : inputLeft_;
                const Delay &fromRight = (t & 1) ? inputLeft_ : inputRight_;
                if (tapsSettled) {
                    earlyLeft += tapGainLeft_[t] * fromLeft.read(tapLeft[t]);
                    earlyRight += tapGainRight_[t] * fromRight.read(tapRight[t]);
                } else {
                    earlyLeft += tapGainLeft_[t] * fromLeft.read(predelay + kTapMsLeft[t] * tapScale);
                    earlyRight += tapGainRight_[t] * fromRight.read(predelay + kTapMsRight[t] * tapScale);
                }
            }
            for (auto &ap : earlySmearLeft_) earlyLeft = ap.process(earlyLeft, smearGain);
            for (auto &ap : earlySmearRight_) earlyRight = ap.process(earlyRight, smearGain);

            // The late reverb hears the pre-delayed input plus some of the early reflections.
            const float preDelayed = tapsSettled ? inputLeft_.read(lateTap) + inputRight_.read(lateTap)
                                                 : inputLeft_.read(predelay + 1.0f) + inputRight_.read(predelay + 1.0f);
            float lateIn = 0.5f * preDelayed + 0.15f * (earlyLeft + earlyRight);
            for (auto &ap : diffusers_) lateIn = ap.process(lateIn, diffuserGain);

            // Feedback delay network: read, decay (two bands + damping), mix, write back.
            for (int i = 0; i < kLines; ++i) {
                const float out = lines_[i].read(lineDelay[i]);
                lineDelay[i] += lineStep[i];
                const float y = decay_[i].process(out, gainLow_[i], gainMid_[i], crossoverCoeff_, dampingCoeff_);
                lineOut_[i] = lineAllpass_[i].process(y, loopAllpassGain);
            }
            float lateLeft = 0.0f, lateRight = 0.0f;
            for (int i = 0; i < kLines; ++i) {
                lateLeft += kOutLeft[i] * lineOut_[i];
                lateRight += kOutRight[i] * lineOut_[i];
            }

            // 8-point Hadamard mix (orthogonal, so the loop is lossless before the decay gains).
            std::array<float, kLines> m = lineOut_;
            for (int h = 1; h < kLines; h <<= 1) {
                for (int a = 0; a < kLines; a += 2 * h) {
                    for (int b = a; b < a + h; ++b) {
                        const float x = m[b], z = m[b + h];
                        m[b] = x + z;
                        m[b + h] = x - z;
                    }
                }
            }
            const float norm = 0.35355339f;  // 1/sqrt(8)
            for (int i = 0; i < kLines; ++i) lines_[i].push(m[i] * norm + kInSign[i] * lateIn * 0.5f);

            const float l = early_ * earlyLeft + late_ * norm * lateLeft;
            const float r = early_ * earlyRight + late_ * norm * lateRight;
            const float mid = 0.5f * (l + r);
            const float side = sideGain * (l - r);
            wetLeft[j] = kWetGain * (mid + side);
            wetRight[j] = kWetGain * (mid - side);
        }
    }
}

} // namespace synth
