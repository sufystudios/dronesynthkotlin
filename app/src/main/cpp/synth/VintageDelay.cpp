// Stereo tape-style delay modelled on the behaviour of Calf Studio Gear's Vintage
// Delay (https://calf-studio-gear.org/doc/Vintage%20Delay.html). Written from scratch
// for DroneSynth from that documented behaviour; no Calf code is used. Ported from
// the iOS app's VintageDelayDSP.mm.
//
// Signal flow, per channel:
//
//   in ──(+)──[Medium filter]──► delay line ──► taps ──► wet (Amount, Width) ──(+)──► out
//         ▲                                       │                              ▲
//         └──────────── Feedback ◄────────────────┘          in ─── Dry ─────────┘
//
// - Timing: one beat (Time, ms) / Subdivide = one unit. The left line delays by
//   Time L units and the right by Time R units (see vintageDelayComputeTaps).
// - Mix mode decides where each channel's feedback is read from:
//     Stereo     - each channel feeds back into itself. The right channel's feedback
//                  is scaled so both sides lose the same level per second, however
//                  different their times are.
//     Ping-Pong  - each channel feeds back into the other.
//     L then R / - the two times chain into one loop of (L + R). One side's echo
//     R then L     lands first, the other's follows; the later side is attenuated as
//                  if it had travelled part-way round the loop.
// - Medium filters what's written into the delay line, so the loss compounds with
//   every repeat, as on tape: Plain (none), Tape (wide band-pass), Old Tape (that
//   band-pass plus a low-pass).
//
// Real-time safety: the delay lines are allocated in init() and only zeroed in
// reset(). process() never allocates or locks.

#include "VintageDelay.h"

#include <algorithm>
#include <cmath>

namespace synth {

// MARK: - Timing

VintageDelayTaps vintageDelayComputeTaps(float timeMs, float subdivide, float timeLeft, float timeRight,
                                         int32_t mixMode, double sampleRate) {
    const double ms = std::clamp(double(timeMs), 10.0, 2000.0);
    const double units = std::clamp(std::round(double(subdivide)), 1.0, 16.0);
    const double left = std::clamp(std::round(double(timeLeft)), 1.0, 16.0);
    const double right = std::clamp(std::round(double(timeRight)), 1.0, 16.0);
    const double samplesPerUnit = ms / 1000.0 * std::max(sampleRate, 1.0) / units;

    double leftSamples = std::max(1.0, std::round(samplesPerUnit * left));
    double rightSamples = std::max(1.0, std::round(samplesPerUnit * right));

    // A read can reach back at most one sample short of the whole line.
    const double longest = double(kVintageDelayBufferLength - 1);
    bool clamped = false;
    const bool chained = mixMode == kMixModeLeftThenRight || mixMode == kMixModeRightThenLeft;
    if (chained) {
        // Both lengths share one loop, so shrink them together to keep their ratio.
        const double loop = leftSamples + rightSamples;
        if (loop > longest) {
            const double scale = longest / loop;
            leftSamples = std::max(1.0, std::floor(leftSamples * scale));
            rightSamples = std::max(1.0, std::floor(rightSamples * scale));
            clamped = true;
        }
    } else {
        if (leftSamples > longest) { leftSamples = longest; clamped = true; }
        if (rightSamples > longest) { rightSamples = longest; clamped = true; }
    }
    return {int32_t(leftSamples), int32_t(rightSamples), clamped};
}

namespace {

/// Flushes denormals and non-finite values to zero. `!(x > tiny)` is also true for
/// NaN, so a single bad sample can never get trapped circulating in the feedback loop.
inline float sanitize(float x) {
    return std::fabs(x) > 1e-15f ? x : 0.0f;
}

/// fb^exponent, with the common exponents special-cased to skip powf.
inline float shapedGain(float feedback, float exponent) {
    if (exponent == 0.0f) return 1.0f;
    if (exponent == 1.0f) return feedback;
    return std::pow(feedback, exponent);
}

} // namespace

// MARK: - Biquad (Robert Bristow-Johnson's Audio EQ Cookbook, direct form I)

void VintageDelay::Biquad::setLowPass(double frequency, double q, double sampleRate) {
    const double w = 2.0 * M_PI * frequency / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double cosw = std::cos(w);
    const double a0 = 1.0 + alpha;
    b0 = (1.0 - cosw) * 0.5 / a0;
    b1 = (1.0 - cosw) / a0;
    b2 = b0;
    a1 = -2.0 * cosw / a0;
    a2 = (1.0 - alpha) / a0;
}

/// The cookbook's "constant 0 dB peak gain" band-pass: unity at the centre
/// frequency, so a repeat loses level only away from it.
void VintageDelay::Biquad::setBandPass(double frequency, double q, double sampleRate) {
    const double w = 2.0 * M_PI * frequency / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    b1 = 0.0;
    b2 = -alpha / a0;
    a1 = -2.0 * std::cos(w) / a0;
    a2 = (1.0 - alpha) / a0;
}

float VintageDelay::Biquad::process(float input) {
    const double x = input;
    double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    if (!(std::fabs(y) > 1e-20)) y = 0.0;
    x2 = x1; x1 = x;
    y2 = y1; y1 = y;
    return float(y);
}

bool VintageDelay::TapSet::operator==(const TapSet &o) const {
    return outLeft == o.outLeft && outRight == o.outRight && feedbackLeft == o.feedbackLeft &&
           feedbackRight == o.feedbackRight && crossFeedback == o.crossFeedback &&
           feedbackExponentLeft == o.feedbackExponentLeft && feedbackExponentRight == o.feedbackExponentRight &&
           amountExponentLeft == o.amountExponentLeft && amountExponentRight == o.amountExponentRight;
}

// MARK: - Lifecycle

void VintageDelay::init(double sampleRate) {
    sampleRate_ = sampleRate;
    lineLeft_.assign(kVintageDelayBufferLength, 0.0f);
    lineRight_.assign(kVintageDelayBufferLength, 0.0f);
    crossfadeStep_ = float(1.0 / std::max(1.0, kTapCrossfadeSeconds * sampleRate));
    mediumGlide_ = float(1.0 - std::exp(-1.0 / std::max(1.0, kMediumGlideSeconds * sampleRate)));
    for (Smoothed *s : {&feedback_, &amount_, &dry_, &width_}) s->setTimeConstant(0.02f, float(sampleRate));

    // Tape: a very wide band-pass centred at 4.5 kHz (Q 0.25), trimming lows and
    // the extreme top. Old Tape adds a 6 kHz Butterworth low-pass after it.
    // The floor keeps both centre frequencies below Nyquist at any sample rate.
    const double rate = std::max(sampleRate, 16000.0);
    bandPassLeft_.setBandPass(4500.0, 0.25, rate);
    bandPassRight_ = bandPassLeft_;
    lowPassLeft_.setLowPass(6000.0, 0.707, rate);
    lowPassRight_ = lowPassLeft_;
    reset();
}

void VintageDelay::reset() {
    std::fill(lineLeft_.begin(), lineLeft_.end(), 0.0f);
    std::fill(lineRight_.begin(), lineRight_.end(), 0.0f);
    bypassed_ = false;
    clearState();
}

void VintageDelay::clearState() {
    writeIndex_ = 0;
    hasTaps_ = false;
    crossfading_ = false;
    bandPassLeft_.clear(); bandPassRight_.clear();
    lowPassLeft_.clear(); lowPassRight_.clear();
    const int m = std::clamp(int(medium), 0, 2);
    for (int i = 0; i < 3; ++i) mediumWeights_[i] = i == m ? 1.0f : 0.0f;
}

void VintageDelay::snapSmoothedValues() {
    for (Smoothed *s : {&feedback_, &amount_, &dry_, &width_}) s->snap(s->target());
}

// MARK: - Routing

VintageDelay::TapSet VintageDelay::makeTaps() const {
    const int32_t mode = mixMode;
    const float leftUnits = std::clamp(std::round(timeLeft), 1.0f, 16.0f);
    const float rightUnits = std::clamp(std::round(timeRight), 1.0f, 16.0f);
    const VintageDelayTaps taps = vintageDelayComputeTaps(time, subdivide, leftUnits, rightUnits, mode, sampleRate_);
    TapSet set;
    switch (mode) {
        case kMixModePingPong:
            set.outLeft = taps.left;
            set.outRight = taps.right;
            // Each line re-records the *other* side's echo.
            set.crossFeedback = true;
            set.feedbackLeft = taps.right;
            set.feedbackRight = taps.left;
            break;
        case kMixModeLeftThenRight:
        case kMixModeRightThenLeft: {
            const int32_t loop = taps.left + taps.right;
            const bool leftFirst = mode == kMixModeLeftThenRight;
            // The first side is heard after its own time, the second after the
            // whole loop; both lines recirculate around the full loop.
            set.outLeft = leftFirst ? taps.left : loop;
            set.outRight = leftFirst ? loop : taps.right;
            set.feedbackLeft = set.feedbackRight = loop;
            const float laterShare = float(leftFirst ? taps.right : taps.left) / float(loop);
            (leftFirst ? set.amountExponentRight : set.amountExponentLeft) = laterShare;
            break;
        }
        case kMixModeStereo:
        default:
            set.outLeft = set.feedbackLeft = taps.left;
            set.outRight = set.feedbackRight = taps.right;
            // Equal decay per second: the right side's repeats are right/left
            // times as far apart as the left's, so each one loses fb^(right/left).
            set.feedbackExponentRight = rightUnits / leftUnits;
            break;
    }
    return set;
}

VintageDelay::Frame VintageDelay::evaluate(const TapSet &set, float feedback, float amount) const {
    const float echoLeft = read(lineLeft_, set.outLeft);
    const float echoRight = read(lineRight_, set.outRight);
    const float recircLeft = set.crossFeedback ? read(lineRight_, set.feedbackLeft) : read(lineLeft_, set.feedbackLeft);
    const float recircRight = set.crossFeedback ? read(lineLeft_, set.feedbackRight) : read(lineRight_, set.feedbackRight);
    return {
        echoLeft * amount * shapedGain(feedback, set.amountExponentLeft),
        echoRight * amount * shapedGain(feedback, set.amountExponentRight),
        recircLeft * shapedGain(feedback, set.feedbackExponentLeft),
        recircRight * shapedGain(feedback, set.feedbackExponentRight),
    };
}

// MARK: - Processing

void VintageDelay::bypass(float *left, float *right, int frames) {
    if (!bypassed_) {
        bypassed_ = true;
        clearedUpTo_ = 0;
    }
    if (clearedUpTo_ < uint32_t(kVintageDelayBufferLength)) {
        const uint32_t n = std::min(kClearStep, uint32_t(kVintageDelayBufferLength) - clearedUpTo_);
        std::fill_n(lineLeft_.begin() + clearedUpTo_, n, 0.0f);
        std::fill_n(lineRight_.begin() + clearedUpTo_, n, 0.0f);
        clearedUpTo_ += n;
    }
    amount_.snap(0.0f);
    feedback_.advance(frames);
    width_.advance(frames);
    for (int i = 0; i < frames; ++i) {
        const float dry = std::clamp(dry_.next(), 0.0f, 4.0f);
        left[i] = sanitize(dry * left[i]);
        right[i] = sanitize(dry * right[i]);
    }
}

void VintageDelay::process(float *left, float *right, int frames) {
    if (lineLeft_.empty() || lineRight_.empty()) return;

    // At wet 0 nothing recorded is heard, so skip the whole delay. -100 dB counts as 0:
    // the smoothed amount only approaches it.
    if (amount_.target() <= 0.0f && amount_.value() < 1e-5f) {
        bypass(left, right, frames);
        return;
    }
    if (bypassed_) {
        // Coming back: start from silence, finishing the clear if it hasn't caught up.
        std::fill(lineLeft_.begin() + clearedUpTo_, lineLeft_.end(), 0.0f);
        std::fill(lineRight_.begin() + clearedUpTo_, lineRight_.end(), 0.0f);
        bypassed_ = false;
        clearState();
    }

    // Stepped controls are read once per block; any change is applied through
    // the tap crossfade below rather than as a jump.
    const TapSet target = makeTaps();
    if (!hasTaps_) {
        currentTaps_ = target;
        hasTaps_ = true;
    } else if (!crossfading_ && target != currentTaps_) {
        nextTaps_ = target;
        crossfading_ = true;
        crossfadePosition_ = 0;
    }
    const int selectedMedium = std::clamp(int(medium), 0, 2);

    for (int i = 0; i < frames; ++i) {
        const float feedback = std::clamp(feedback_.next(), 0.0f, 1.0f);
        const float amount = std::clamp(amount_.next(), 0.0f, 4.0f);
        const float dry = std::clamp(dry_.next(), 0.0f, 4.0f);
        const float width = std::clamp(width_.next(), -1.0f, 1.0f);

        const float inLeft = left[i];
        const float inRight = right[i];

        Frame frame = evaluate(currentTaps_, feedback, amount);
        if (crossfading_) {
            const Frame incoming = evaluate(nextTaps_, feedback, amount);
            const float x = crossfadePosition_;
            frame.wetLeft += x * (incoming.wetLeft - frame.wetLeft);
            frame.wetRight += x * (incoming.wetRight - frame.wetRight);
            frame.feedbackLeft += x * (incoming.feedbackLeft - frame.feedbackLeft);
            frame.feedbackRight += x * (incoming.feedbackRight - frame.feedbackRight);
            crossfadePosition_ += crossfadeStep_;
            if (crossfadePosition_ >= 1.0f) {
                currentTaps_ = nextTaps_;
                crossfading_ = false;
            }
        }

        // Record input plus feedback, through the Medium filters. All filters run
        // continuously so switching media only changes the blend, not the state.
        const float recordLeft = inLeft + frame.feedbackLeft;
        const float recordRight = inRight + frame.feedbackRight;
        const float tapeLeft = bandPassLeft_.process(recordLeft);
        const float tapeRight = bandPassRight_.process(recordRight);
        const float oldTapeLeft = lowPassLeft_.process(tapeLeft);
        const float oldTapeRight = lowPassRight_.process(tapeRight);
        for (int m = 0; m < 3; ++m) {
            const float goal = m == selectedMedium ? 1.0f : 0.0f;
            mediumWeights_[m] += mediumGlide_ * (goal - mediumWeights_[m]);
        }
        lineLeft_[writeIndex_] = sanitize(mediumWeights_[0] * recordLeft + mediumWeights_[1] * tapeLeft + mediumWeights_[2] * oldTapeLeft);
        lineRight_[writeIndex_] = sanitize(mediumWeights_[0] * recordRight + mediumWeights_[1] * tapeRight + mediumWeights_[2] * oldTapeRight);
        writeIndex_ = (writeIndex_ + 1) & kMask;

        // Width: 1 keeps the sides as they are, 0 sums them to mono, -1 swaps them.
        const float blend = (1.0f - width) * 0.5f;
        const float wetLeft = frame.wetLeft + blend * (frame.wetRight - frame.wetLeft);
        const float wetRight = frame.wetRight + blend * (frame.wetLeft - frame.wetRight);

        left[i] = sanitize(dry * inLeft + wetLeft);
        right[i] = sanitize(dry * inRight + wetRight);
    }
}

} // namespace synth
