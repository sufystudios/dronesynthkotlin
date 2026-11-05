#include "Looper.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace synth {

namespace {

constexpr int kKernelZeroCrossings = 12;
constexpr int kKernelResolution = 256;

/// Half of a Blackman-windowed sinc, sampled kKernelResolution times per zero crossing.
std::vector<float> makeKernel() {
    std::vector<float> kernel(size_t(kKernelZeroCrossings * kKernelResolution) + 2, 0.0f);
    for (int i = 0; i <= kKernelZeroCrossings * kKernelResolution; ++i) {
        const double x = double(i) / kKernelResolution;
        const double t = x / kKernelZeroCrossings;
        const double sinc = i == 0 ? 1.0 : std::sin(M_PI * x) / (M_PI * x);
        kernel[size_t(i)] = float(sinc * (0.42 + 0.5 * std::cos(M_PI * t) + 0.08 * std::cos(2.0 * M_PI * t)));
    }
    return kernel;
}

/// Resamples a stereo loop to `dstLength` frames. Reading wraps around the loop's ends,
/// so its wrap stays seamless. Not real-time safe.
void resampleLoop(const float *src, int32_t srcLength, float *dst, int32_t dstLength) {
    static const std::vector<float> kernel = makeKernel();
    const double step = double(srcLength) / double(dstLength);
    // Going down in rate, stay under the new Nyquist; going up, nothing needs removing.
    const double cutoff = std::min(1.0, double(dstLength) / double(srcLength)) * 0.97;
    const int width = int(std::ceil(kKernelZeroCrossings / cutoff));
    const float gain = float(cutoff);
    for (int32_t i = 0; i < dstLength; ++i) {
        const double position = double(i) * step;
        const int32_t centre = int32_t(position);
        const double fraction = position - double(centre);
        float left = 0.0f, right = 0.0f;
        for (int k = 1 - width; k <= width; ++k) {
            const double scaled = std::fabs((double(k) - fraction) * cutoff) * kKernelResolution;
            const int index = int(scaled);
            if (index >= kKernelZeroCrossings * kKernelResolution) continue;
            const float blend = float(scaled - double(index));
            const float h = kernel[size_t(index)] + blend * (kernel[size_t(index) + 1] - kernel[size_t(index)]);
            int32_t frame = (centre + k) % srcLength;
            if (frame < 0) frame += srcLength;
            left += h * src[2 * frame];
            right += h * src[2 * frame + 1];
        }
        dst[2 * i] = left * gain;
        dst[2 * i + 1] = right * gain;
    }
}

} // namespace

void Looper::prepare(double sampleRate) {
    if (sampleRate == sampleRate_) return;
    // A new output at another rate: anything being captured ends here, and what's
    // already there is converted, so the loop plays on at the same pitch and speed.
    if (recording_) {
        const int32_t end = finishing_ ? loopEnd_ : recorded_a_ - crossfadeFrames_;
        loopEnd_ = std::min(end, recorded_a_ - crossfadeFrames_);
        finishRecording();
    }
    stopOverdub();

    const int32_t capacity = int32_t(kMaxSeconds * sampleRate);
    std::vector<float> bufferA(size_t(capacity) * 2, 0.0f);
    std::vector<float> bufferB(size_t(capacity) * 2, 0.0f);
    if (length_a_ > 0) {
        const double ratio = sampleRate / sampleRate_;
        const int32_t length = std::clamp(int32_t(std::lround(double(length_a_) * ratio)), 1, capacity);
        resampleLoop(current_, length_a_, bufferA.data(), length);
        if (canUndo_) resampleLoop(undo_, length_a_, bufferB.data(), length);
        position_a_ = std::min(int32_t(double(position_a_) * ratio), length - 1);
        length_a_ = length;
    }
    bufferA_.swap(bufferA);
    bufferB_.swap(bufferB);
    current_ = bufferA_.data();
    undo_ = bufferB_.data();
    sampleRate_ = sampleRate;
    capacity_ = capacity;
    recorded_a_ = 0;
    crossfadeFrames_ = std::max(1, int32_t(0.02 * sampleRate));
    playGain_.setTimeConstant(0.01f, float(sampleRate));
    playGain_.snap(0.0f);
    publish();
}

void Looper::request(Command command) {
    // Several threads may send commands (UI, notification actions); the audio
    // thread is the only consumer and never takes this lock.
    std::lock_guard<std::mutex> lock(producerLock_);
    commands_.push(command);
}

// MARK: - Audio thread

void Looper::handle(Command command) {
    switch (command) {
        case ToggleRecord:
            if (recording_) {
                // Keep recording a little longer to get the crossfade material.
                if (!finishing_) {
                    finishing_ = true;
                    loopEnd_ = recorded_a_;
                }
            } else {
                // A new take replaces any existing loop.
                stopOverdub();
                recording_ = true;
                playing_ = false;
                canUndo_ = false;
                length_a_ = 0;
                recorded_a_ = 0;
                position_a_ = 0;
            }
            break;
        case TogglePlay:
            if (length_a_ == 0 || recording_) break;
            playing_ = !playing_;
            // Stopping keeps the position, so the next PLAY resumes where it left off.
            if (!playing_) stopOverdub();
            break;
        case ToggleOverdub:
            if (overdubbing_ || snapshotting_) {
                stopOverdub();
            } else if (length_a_ > 0 && !recording_) {
                snapshotting_ = true;
                snapshotCursor_ = 0;
                canUndo_ = false;
                playing_ = true;
            }
            break;
        case Undo:
            if (!canUndo_) break;
            stopOverdub();
            std::swap(current_, undo_);
            canUndo_ = false;
            break;
        case Clear:
            recording_ = finishing_ = playing_ = false;
            stopOverdub();
            canUndo_ = false;
            length_a_ = position_a_ = recorded_a_ = 0;
            break;
        case Enable:
            enabled_ = true;
            break;
        case Disable:
            enabled_ = false;
            // Switching the looper off also stops it, as Play/Stop does: a muted loop that
            // kept running would keep the position ring turning and the app "sounding".
            playing_ = false;
            stopOverdub();
            break;
    }
}

void Looper::stopOverdub() {
    // An overdub session that got as far as recording leaves a complete snapshot.
    if (overdubbing_) canUndo_ = true;
    overdubbing_ = false;
    snapshotting_ = false;
}

void Looper::finishRecording() {
    recording_ = false;
    finishing_ = false;
    const int32_t length = loopEnd_;
    if (length <= crossfadeFrames_ * 2) {
        length_a_ = 0;  // too short to be a loop
        return;
    }
    // Equal-power crossfade of the material recorded past the end into the start:
    // playback leaving frame length-1 continues into what naturally followed it.
    for (int32_t i = 0; i < crossfadeFrames_; ++i) {
        const float t = (float(i) + 0.5f) / float(crossfadeFrames_);
        const float in = std::sin(t * float(M_PI) * 0.5f);
        const float out = std::cos(t * float(M_PI) * 0.5f);
        for (int ch = 0; ch < 2; ++ch) {
            float &head = current_[2 * i + ch];
            head = head * in + current_[2 * (length + i) + ch] * out;
        }
    }
    length_a_ = length;
    position_a_ = 0;
    playing_ = true;
}

void Looper::process(const float *inL, const float *inR, float *outL, float *outR, int frames) {
    if (!current_) return;
    Command command;
    while (commands_.pop(command)) handle(command);

    // Copy part of the loop into the undo snapshot; overdub writes begin once it's complete.
    if (snapshotting_) {
        const int32_t n = std::min(kSnapshotFramesPerCallback, length_a_ - snapshotCursor_);
        std::memcpy(undo_ + 2 * snapshotCursor_, current_ + 2 * snapshotCursor_, sizeof(float) * 2 * size_t(n));
        snapshotCursor_ += n;
        if (snapshotCursor_ >= length_a_) {
            snapshotting_ = false;
            overdubbing_ = true;
        }
    }

    playGain_.setTarget(playing_ && enabled_ ? 1.0f : 0.0f);
    // End a finished fade-out exactly. With denormals flushed and no fused multiply-add
    // (32-bit ARM), the fade otherwise stalls just above zero and the loop never parks.
    if (playGain_.target() == 0.0f && playGain_.value() < 1e-5f) playGain_.snap(0.0f);
    for (int i = 0; i < frames; ++i) {
        const float l = inL[i];
        const float r = inR[i];

        if (recording_) {
            current_[2 * recorded_a_] = l;
            current_[2 * recorded_a_ + 1] = r;
            ++recorded_a_;
            // At the 30 s limit, stop and leave room for the crossfade material.
            if (!finishing_ && recorded_a_ >= capacity_ - crossfadeFrames_) {
                finishing_ = true;
                loopEnd_ = recorded_a_;
            }
            if (finishing_ && recorded_a_ >= loopEnd_ + crossfadeFrames_) finishRecording();
        }

        const float gain = playGain_.next();
        if (length_a_ > 0 && (playing_ || gain > 0.0f)) {
            float *frame = current_ + 2 * position_a_;
            const float pl = frame[0];
            const float pr = frame[1];
            // Overdub sums this pass into the loop - each lap layers on the last.
            if (overdubbing_) {
                frame[0] = pl + l;
                frame[1] = pr + r;
            }
            outL[i] = l + pl * gain;
            outR[i] = r + pr * gain;
            // Keep moving through a stop's fade-out rather than holding one frame.
            if (++position_a_ >= length_a_) position_a_ = 0;
        } else {
            outL[i] = l;
            outR[i] = r;
        }
    }
    publish();
}

void Looper::publish() {
    int32_t f = 0;
    if (length_a_ > 0) f |= kHasLoop;
    if (recording_) f |= kRecording;
    if (playing_) f |= kPlaying;
    if (overdubbing_ || snapshotting_) f |= kOverdubbing;
    if (canUndo_) f |= kCanUndo;
    if (enabled_) f |= kEnabled;
    flags_.store(f, std::memory_order_relaxed);
    length_.store(length_a_, std::memory_order_relaxed);
    position_.store(position_a_, std::memory_order_relaxed);
    recorded_.store(recorded_a_, std::memory_order_relaxed);
}

} // namespace synth
