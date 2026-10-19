#include "PlateReverb.h"

#include <algorithm>
#include <cmath>

namespace synth {

using namespace reverb;

namespace {

/// Dattorro's lengths, in samples at his 29.761 kHz.
constexpr double kPaperRate = 29761.0;
constexpr int kDiffuser[4] = {142, 107, 379, 277};
constexpr float kDiffuserGain[4] = {0.75f, 0.75f, 0.625f, 0.625f};
constexpr float kDecayDiffusion1 = 0.70f;
constexpr float kExcursion = 16.0f;  // modulated allpass swing
constexpr float kModRateHz = 1.0f;

struct HalfLengths { float modAllpass, delay1, allpass, delay2; };
constexpr HalfLengths kLeft = {672, 4453, 1800, 3720};
constexpr HalfLengths kRight = {908, 4217, 2656, 3163};

/// The size knob scales the tank from half to one and a half times the paper's plate.
constexpr float kMinScale = 0.5f, kMaxScale = 1.5f;

/// Sets the wet level to match the hall, so switching type keeps the balance.
constexpr float kWetGain = 0.3f;

} // namespace

void PlateReverb::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    rateScale_ = float(sampleRate / kPaperRate);
    const auto len = [&](float paperSamples) { return int(std::ceil(paperSamples * rateScale_ * kMaxScale)) + 4; };
    const int excursion = int(std::ceil(kExcursion * rateScale_)) + 4;

    input_.allocate(int(sampleRate * 0.1) + 8);  // pre-delay up to 100 ms
    for (int k = 0; k < 4; ++k) diffusers_[k].allocate(int(kDiffuser[k] * rateScale_));
    for (auto [half, lengths] : {std::pair{&left_, kLeft}, std::pair{&right_, kRight}}) {
        half->modAllpass.allocate(1, len(lengths.modAllpass) + excursion);
        half->delay1.allocate(len(lengths.delay1));
        half->allpass.allocate(1, len(lengths.allpass));
        half->delay2.allocate(len(lengths.delay2));
        half->decay1 = DecayStage{};
        half->decay2 = DecayStage{};
        half->feedback = 0.0f;
    }
    lfoPhase_ = 0.0;
    smooth_ = 1.0f - std::exp(-float(kSubBlock) / float(0.1 * sampleRate));
    primed_ = false;
    derivedFrom_.fill(-1.0f);
}

void PlateReverb::clear() {
    input_.clear();
    for (auto &a : diffusers_) a.clear();
    for (Half *half : {&left_, &right_}) {
        half->modAllpass.clear();
        half->delay1.clear();
        half->allpass.clear();
        half->delay2.clear();
        half->decay1 = DecayStage{};
        half->decay2 = DecayStage{};
        half->feedback = 0.0f;
    }
    primed_ = false;  // the settings may have moved meanwhile: snap to them
}

void PlateReverb::updateCoefficients() {
    const float scale = kMinScale + (kMaxScale - kMinScale) * std::clamp(target_.size, 0.0f, 1.0f);
    const float predelay = std::clamp(target_.predelayMs, 0.0f, 100.0f) * float(sampleRate_) / 1000.0f;

    const float k = primed_ ? smooth_ : 1.0f;
    auto glide = [k](float &value, float to) { value += k * (to - value); };
    glide(scale_, scale);
    glide(predelay_, predelay);
    glide(applied_.rt60Low, std::clamp(target_.rt60Low, 0.1f, 30.0f));
    glide(applied_.rt60Mid, std::clamp(target_.rt60Mid, 0.1f, 30.0f));
    glide(applied_.crossoverHz, target_.crossoverHz);
    glide(applied_.dampingHz, target_.dampingHz);
    primed_ = true;

    // The pow() and exp() below are costly on slow phones; skip them once the glides settle.
    const std::array<float, 5> inputs = {scale_, applied_.rt60Low, applied_.rt60Mid,
                                         applied_.crossoverHz, applied_.dampingHz};
    if (inputs == derivedFrom_) return;
    derivedFrom_ = inputs;

    // Each half has two decay points; give each the loss for the stretch of tank
    // before it, so a full trip round the tank loses -60 dB × (trip time / RT60).
    // The halves differ slightly in length; their average sets the gains.
    const float sr = float(sampleRate_);
    const float stretch = rateScale_ * scale_;
    const float seg1 = 0.5f * (kLeft.modAllpass + kLeft.delay1 + kRight.modAllpass + kRight.delay1) * stretch;
    const float seg2 = 0.5f * (kLeft.allpass + kLeft.delay2 + kRight.allpass + kRight.delay2) * stretch;
    auto gain = [sr](float samples, float rt60) { return std::pow(10.0f, -3.0f * samples / (rt60 * sr)); };
    gainMid1_ = gain(seg1, applied_.rt60Mid);
    gainLow1_ = gain(seg1, applied_.rt60Low);
    gainMid2_ = gain(seg2, applied_.rt60Mid);
    gainLow2_ = gain(seg2, applied_.rt60Low);
    // Dattorro ties the second tank diffusion to the decay.
    decayDiffusion2_ = std::clamp(gainMid1_ + 0.15f, 0.25f, 0.5f);
    crossoverCoeff_ = onePoleCoeff(applied_.crossoverHz, sampleRate_);
    dampingCoeff_ = onePoleCoeff(applied_.dampingHz, sampleRate_);
}

void PlateReverb::process(const float *inLeft, const float *inRight, float *wetLeft, float *wetRight, int frames) {
    for (int start = 0; start < frames; start += kSubBlock) {
        const int n = std::min(kSubBlock, frames - start);
        const bool wasPrimed = primed_;
        float scale = scale_, predelay = predelay_;
        updateCoefficients();
        if (!wasPrimed) {
            scale = scale_;
            predelay = predelay_;
        }
        // Ramped a sample at a time so a size change never jumps the read positions.
        const float scaleStep = (scale_ - scale) / float(n);
        const float predelayStep = (predelay_ - predelay) / float(n);
        const double phaseStep = kModRateHz / sampleRate_;
        const float excursion = kExcursion * rateScale_;

        for (int j = start; j < start + n; ++j) {
            scale += scaleStep;
            predelay += predelayStep;
            const float s = rateScale_ * scale;  // paper samples -> our samples

            // Mono in, pre-delayed and diffused.
            input_.push(0.5f * (inLeft[j] + inRight[j]));
            float x = input_.read(predelay + 1.0f);
            for (int k = 0; k < 4; ++k) x = diffusers_[k].process(x, kDiffuserGain[k]);

            // The tank: each half takes the input plus the other half's output.
            lfoPhase_ += phaseStep;
            if (lfoPhase_ >= 1.0) lfoPhase_ -= 1.0;
            const float lfo = kTwoPi * float(lfoPhase_);
            const float modLeft = excursion * std::sin(lfo);
            const float modRight = excursion * std::cos(lfo);

            const float intoLeft = x + right_.feedback;
            const float intoRight = x + left_.feedback;
            for (auto [half, lengths, in, mod] : {std::tuple{&left_, kLeft, intoLeft, modLeft},
                                                  std::tuple{&right_, kRight, intoRight, modRight}}) {
                float y = half->modAllpass.processAt(in, -kDecayDiffusion1, lengths.modAllpass * s + mod);
                half->delay1.push(y);
                y = half->delay1.read(lengths.delay1 * s);
                y = half->decay1.process(y, gainLow1_, gainMid1_, crossoverCoeff_, dampingCoeff_);
                y = half->allpass.processAt(y, decayDiffusion2_, lengths.allpass * s);
                half->delay2.push(y);
                y = half->delay2.read(lengths.delay2 * s);
                half->feedback = half->decay2.process(y, gainLow2_, gainMid2_, crossoverCoeff_, dampingCoeff_);
            }

            // Dattorro's output taps, spread across both halves for each side.
            auto tap = [s](const Delay &d, float paperSamples) { return d.read(paperSamples * s); };
            const float l = tap(right_.delay1, 266) + tap(right_.delay1, 2974) - tap(right_.allpass.delay, 1913) +
                            tap(right_.delay2, 1996) - tap(left_.delay1, 1990) - tap(left_.allpass.delay, 187) -
                            tap(left_.delay2, 1066);
            const float r = tap(left_.delay1, 353) + tap(left_.delay1, 3627) - tap(left_.allpass.delay, 1228) +
                            tap(left_.delay2, 2673) - tap(right_.delay1, 2111) - tap(right_.allpass.delay, 335) -
                            tap(right_.delay2, 121);
            wetLeft[j] = kWetGain * 0.6f * l;
            wetRight[j] = kWetGain * 0.6f * r;
        }
    }
}

} // namespace synth
