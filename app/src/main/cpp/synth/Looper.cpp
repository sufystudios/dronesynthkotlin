#include "Looper.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace synth {

void Looper::prepare(double sampleRate) {
    capacity_ = int32_t(kMaxSeconds * sampleRate);
    bufferA_.assign(size_t(capacity_) * 2, 0.0f);
    bufferB_.assign(size_t(capacity_) * 2, 0.0f);
    current_ = bufferA_.data();
    undo_ = bufferB_.data();
    crossfadeFrames_ = std::max(1, int32_t(0.02 * sampleRate));
    playGain_.setTimeConstant(0.01f, float(sampleRate));
    playGain_.snap(0.0f);
    length_a_ = position_a_ = recorded_a_ = 0;
    recording_ = finishing_ = playing_ = overdubbing_ = snapshotting_ = canUndo_ = false;
    enabled_ = true;
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
