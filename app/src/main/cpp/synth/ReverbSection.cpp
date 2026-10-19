#include "ReverbSection.h"

#include <algorithm>
#include <cmath>

namespace synth {

namespace {
constexpr float kBodyHz = 250.0f;
constexpr float kShimmerHz = 6000.0f;
constexpr float kEqQ = 0.7f;
} // namespace

void ReverbSection::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    hall_.prepare(sampleRate);
    plate_.prepare(sampleRate);
    for (auto *buffer : {&hallLeft_, &hallRight_, &plateLeft_, &plateRight_}) buffer->assign(kChunk, 0.0f);
    bodyLeft_ = bodyRight_ = shimmerLeft_ = shimmerRight_ = reverb::Biquad{};
    eqBody_ = eqShimmer_ = 0.0f;
    eqSmooth_ = 1.0f - std::exp(-float(kEqStep) / float(0.1 * sampleRate));
    mixSmooth_ = 1.0f - std::exp(-1.0f / float(0.02 * sampleRate));   // ~20 ms
    typeSmooth_ = 1.0f - std::exp(-1.0f / float(0.08 * sampleRate));  // ~80 ms crossfade
    primed_ = false;
    hallRunning_ = plateRunning_ = true;
}

void ReverbSection::set(const ReverbSettings &settings) {
    settings_ = settings;
    hall_.set(settings);
    plate_.set(settings);
}

void ReverbSection::updateEq() {
    // The gains glide (~100 ms) in small steps: a peaking biquad takes those without
    // clicking, where a jump straight to +15 dB would.
    const float k = primed_ ? eqSmooth_ : 1.0f;
    const float body = eqBody_ + k * (settings_.bodyDb - eqBody_);
    const float shimmer = eqShimmer_ + k * (settings_.shimmerDb - eqShimmer_);
    if (std::fabs(body - eqBody_) > 1e-4f || !primed_) {
        eqBody_ = body;
        bodyLeft_.setPeaking(kBodyHz, kEqQ, eqBody_, float(sampleRate_));
        bodyRight_.setPeaking(kBodyHz, kEqQ, eqBody_, float(sampleRate_));
    }
    if (std::fabs(shimmer - eqShimmer_) > 1e-4f || !primed_) {
        eqShimmer_ = shimmer;
        shimmerLeft_.setPeaking(kShimmerHz, kEqQ, eqShimmer_, float(sampleRate_));
        shimmerRight_.setPeaking(kShimmerHz, kEqQ, eqShimmer_, float(sampleRate_));
    }
}

template <class Reverb>
void ReverbSection::runOrIdle(Reverb &reverb, bool run, bool &running, const float *inLeft, const float *inRight,
                              std::vector<float> &wetLeft, std::vector<float> &wetRight, int frames) {
    if (run) {
        reverb.process(inLeft, inRight, wetLeft.data(), wetRight.data(), frames);
    } else {
        if (running) reverb.clear();
        std::fill_n(wetLeft.begin(), frames, 0.0f);
        std::fill_n(wetRight.begin(), frames, 0.0f);
    }
    running = run;
}

void ReverbSection::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, int frames) {
    const float plateTarget = settings_.type == ReverbType::Plate ? 1.0f : 0.0f;
    if (!primed_) {
        mix_ = settings_.mix;
        plateAmount_ = plateTarget;
        updateEq();
        primed_ = true;
    }
    for (int start = 0; start < frames; start += kChunk) {
        const int n = std::min(kChunk, frames - start);
        // Settled glides snap to their target, so "off" and "fully the other type" are exact.
        if (std::fabs(mix_ - settings_.mix) < 1e-4f) mix_ = settings_.mix;
        if (std::fabs(plateAmount_ - plateTarget) < 1e-4f) plateAmount_ = plateTarget;

        const bool silent = mix_ == 0.0f && settings_.mix <= 0.0f;
        const bool runHall = !silent && (plateAmount_ < 1.0f || plateTarget < 1.0f);
        const bool runPlate = !silent && (plateAmount_ > 0.0f || plateTarget > 0.0f);
        runOrIdle(hall_, runHall, hallRunning_, inLeft + start, inRight + start, hallLeft_, hallRight_, n);
        runOrIdle(plate_, runPlate, plateRunning_, inLeft + start, inRight + start, plateLeft_, plateRight_, n);
        if (silent) {
            // Bypassed: pass the dry signal straight through.
            for (auto *eq : {&bodyLeft_, &bodyRight_, &shimmerLeft_, &shimmerRight_}) eq->clear();
            if (outLeft != inLeft) std::copy_n(inLeft + start, n, outLeft + start);
            if (outRight != inRight) std::copy_n(inRight + start, n, outRight + start);
            continue;
        }
        for (int i = 0; i < n; ++i) {
            if (i % kEqStep == 0) updateEq();
            plateAmount_ += typeSmooth_ * (plateTarget - plateAmount_);
            mix_ += mixSmooth_ * (settings_.mix - mix_);
            // Equal-power crossfade, so the level holds steady mid-switch.
            const float plateGain = std::sin(1.5707963f * plateAmount_);
            const float hallGain = std::cos(1.5707963f * plateAmount_);
            const float wetLeft = shimmerLeft_.process(bodyLeft_.process(hallGain * hallLeft_[i] + plateGain * plateLeft_[i]));
            const float wetRight = shimmerRight_.process(bodyRight_.process(hallGain * hallRight_[i] + plateGain * plateRight_[i]));
            const int j = start + i;
            const float dryLeft = inLeft[j], dryRight = inRight[j];
            outLeft[j] = (1.0f - mix_) * dryLeft + mix_ * wetLeft;
            outRight[j] = (1.0f - mix_) * dryRight + mix_ * wetRight;
        }
    }
}

} // namespace synth
