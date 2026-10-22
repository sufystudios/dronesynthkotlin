//
// Carries the ARP/SEQ module (StepSequencer, shared unchanged with the iOS app)
// between threads, as the iOS app's StepSequencerDSP.mm does.
//
// The UI thread posts settings, REST/TIE, CLEAR and whole patterns into a
// lock-free queue; the audio thread drains it at the start of each callback and
// calls the sequencer directly from then on (keys reach it through the engine's
// note queue). Recorded patterns travel back for saving behind a sequence lock.
//

#ifndef DRONESYNTH_SEQUENCERBRIDGE_H
#define DRONESYNTH_SEQUENCERBRIDGE_H

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

#include "EventQueue.h"
#include "StepSequencer.h"

namespace synth {

class SequencerBridge {
public:
    static constexpr int kMaxSteps = StepSequencer::kMaxSteps;
    static constexpr int kPatterns = StepSequencer::kPatterns;

    // --- UI thread (any non-audio thread; posts are serialised here)

    /// Each returns false if the queue is full (the audio thread isn't draining it).
    bool postSettings(const SequencerSettings &settings);
    bool postRestOrTie();
    bool postClearPattern(int pattern);
    /// Replaces a pattern (from a preset or the saved patch) at the next callback.
    bool postPattern(int pattern, const SequencerStep *steps, int length);
    /// A pattern as it stands: what was last posted, until the audio thread has taken
    /// it, then its latest copy from the audio thread (steps recorded in REC included).
    int readPattern(int pattern, SequencerStep *steps, int capacity);

    /// For the UI, from any thread.
    int currentStep() const { return sequencer_.currentStep(); }
    bool isRunning() const { return sequencer_.isRunning(); }
    uint32_t clockTicks() const { return sequencer_.clockTicks(); }

    // --- audio thread

    void prepare(double sampleRate) { sequencer_.prepare(sampleRate); }
    /// Applies everything posted since the last call, in order.
    void drain();
    /// The events due in the next `frames` frames (see StepSequencer::advance).
    int advance(int frames, SequencerEvent *out, int capacity);
    StepSequencer &sequencer() { return sequencer_; }
    /// The settings last drained.
    const SequencerSettings &settings() const { return settings_; }

private:
    struct Command {
        enum Type : uint8_t { Settings, RestOrTie, ClearPattern, SetPattern };
        Type type;
        int32_t a, b;
        SequencerSettings settings;
        uint32_t generation;  // SetPattern, ClearPattern: which post this is
    };

    /// A pattern's steps, as handed between threads.
    struct PatternCopy {
        std::array<SequencerStep, kMaxSteps> steps{};
        int length = 0;
    };

    void publishChangedPatterns();
    static int copyOut(const PatternCopy &pattern, SequencerStep *steps, int capacity);

    StepSequencer sequencer_;
    SequencerSettings settings_{};
    SpscQueue<Command, 512> commands_;
    std::mutex producerLock_;

    // Patterns posted from the UI thread travel in these buffers: the UI thread fills
    // a free one and queues a command naming it; the audio thread copies it into the
    // sequencer and frees it. Enough for several preset loads in a row.
    static constexpr int kPostSlots = 8;
    std::array<PatternCopy, kPostSlots> postSlots_{};
    std::array<std::atomic<bool>, kPostSlots> postSlotBusy_{};

    // What the UI thread last posted for each pattern, and which post that was;
    // until the audio thread has taken it, that's the pattern's true contents.
    std::array<PatternCopy, kPatterns> lastPosted_{};
    std::array<uint32_t, kPatterns> postedGeneration_{};
    std::array<std::atomic<uint32_t>, kPatterns> appliedGeneration_{};

    // The audio thread publishes each pattern here whenever it changes (recorded,
    // cleared or loaded), behind a sequence lock, for the UI thread to save.
    std::array<PatternCopy, kPatterns> published_{};
    std::array<std::atomic<uint32_t>, kPatterns> publishSequence_{};
    std::array<uint32_t, kPatterns> publishedRevision_{};
};

} // namespace synth

#endif //DRONESYNTH_SEQUENCERBRIDGE_H
