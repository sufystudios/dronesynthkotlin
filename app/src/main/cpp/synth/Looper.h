//
// The Looper (the iOS app's Looper drawer): records up to 30 s of the synth's own
// output, loops it back alongside the live signal, and overdubs further passes on
// top. UNDO returns to the take as it was before the current overdub session.
//
// Everything that touches the loop audio runs on the audio thread. The UI thread
// only queues commands and reads a few atomics for display. Buffers are allocated
// up front; the undo snapshot is copied in chunks across callbacks before an
// overdub starts recording, so no callback ever does a large copy.
//

#ifndef DRONESYNTH_LOOPER_H
#define DRONESYNTH_LOOPER_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "EventQueue.h"
#include "Modulators.h"

namespace synth {

class Looper {
public:
    static constexpr double kMaxSeconds = 30.0;

    enum Command : uint8_t { ToggleRecord, TogglePlay, ToggleOverdub, Undo, Clear, Enable, Disable };

    /// Bit flags returned by state() for the UI.
    enum StateFlag : int32_t {
        kHasLoop = 1 << 0,
        kRecording = 1 << 1,
        kPlaying = 1 << 2,
        kOverdubbing = 1 << 3,
        kCanUndo = 1 << 4,
        kEnabled = 1 << 5,
    };

    /// Allocates the loop buffers. Not real-time safe; call before audio starts.
    void prepare(double sampleRate);

    /// UI thread. Queues a command for the audio thread.
    void request(Command command);

    /// Audio thread. Captures `inL/inR` (the synth's output) as the commands
    /// require, then adds loop playback into `outL/outR`. In and out may alias.
    void process(const float *inL, const float *inR, float *outL, float *outR, int frames);

    int32_t flags() const { return flags_.load(std::memory_order_relaxed); }
    int32_t lengthFrames() const { return length_.load(std::memory_order_relaxed); }
    int32_t positionFrames() const { return position_.load(std::memory_order_relaxed); }
    int32_t recordedFrames() const { return recorded_.load(std::memory_order_relaxed); }

private:
    void handle(Command command);
    void finishRecording();
    void stopOverdub();
    void publish();

    static constexpr int32_t kSnapshotFramesPerCallback = 65536;

    std::mutex producerLock_;
    SpscQueue<Command, 32> commands_;

    // Two equal buffers; `current_` is the loop, `undo_` the pre-overdub snapshot.
    std::vector<float> bufferA_, bufferB_;
    float *current_ = nullptr;
    float *undo_ = nullptr;
    int32_t capacity_ = 0;  // frames
    /// Length of the wrap crossfade: recording runs this much past the loop's end,
    /// and that continuation is blended into the loop's start so the wrap is seamless.
    int32_t crossfadeFrames_ = 960;

    // Audio-thread state.
    int32_t length_a_ = 0;
    int32_t position_a_ = 0;
    int32_t recorded_a_ = 0;
    bool recording_ = false;
    bool finishing_ = false;  // recording the crossfade material past the loop end
    int32_t loopEnd_ = 0;
    bool playing_ = false;
    bool overdubbing_ = false;
    bool snapshotting_ = false;  // copying current_ -> undo_ before overdub writes begin
    int32_t snapshotCursor_ = 0;
    bool canUndo_ = false;
    bool enabled_ = true;
    Smoothed playGain_;

    // Published for the UI.
    std::atomic<int32_t> flags_{kEnabled};
    std::atomic<int32_t> length_{0};
    std::atomic<int32_t> position_{0};
    std::atomic<int32_t> recorded_{0};
};

} // namespace synth

#endif //DRONESYNTH_LOOPER_H
