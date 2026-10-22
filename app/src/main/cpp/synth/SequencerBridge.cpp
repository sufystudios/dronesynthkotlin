#include "SequencerBridge.h"

#include <algorithm>

namespace synth {

// MARK: - UI thread

bool SequencerBridge::postSettings(const SequencerSettings &settings) {
    std::lock_guard<std::mutex> lock(producerLock_);
    return commands_.push({Command::Settings, 0, 0, settings, 0});
}

bool SequencerBridge::postRestOrTie() {
    std::lock_guard<std::mutex> lock(producerLock_);
    return commands_.push({Command::RestOrTie, 0, 0, {}, 0});
}

bool SequencerBridge::postClearPattern(int pattern) {
    std::lock_guard<std::mutex> lock(producerLock_);
    if (pattern < 0 || pattern >= kPatterns) return false;
    // Reads before the audio thread gets to it see it empty already.
    const uint32_t generation = postedGeneration_[pattern] + 1;
    if (!commands_.push({Command::ClearPattern, pattern, 0, {}, generation})) return false;
    lastPosted_[pattern].length = 0;
    postedGeneration_[pattern] = generation;
    return true;
}

bool SequencerBridge::postPattern(int pattern, const SequencerStep *steps, int length) {
    std::lock_guard<std::mutex> lock(producerLock_);
    if (pattern < 0 || pattern >= kPatterns) return false;
    int slot = -1;
    for (int i = 0; i < kPostSlots; ++i) {
        if (!postSlotBusy_[i].load(std::memory_order_acquire)) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return false;  // the audio thread hasn't taken the earlier ones

    PatternCopy &copy = postSlots_[slot];
    copy.length = steps ? std::clamp(length, 0, kMaxSteps) : 0;
    std::copy(steps, steps + copy.length, copy.steps.begin());
    postSlotBusy_[slot].store(true, std::memory_order_relaxed);

    const uint32_t generation = postedGeneration_[pattern] + 1;
    if (!commands_.push({Command::SetPattern, pattern, slot, {}, generation})) {
        postSlotBusy_[slot].store(false, std::memory_order_relaxed);
        return false;
    }
    lastPosted_[pattern] = copy;
    postedGeneration_[pattern] = generation;
    return true;
}

int SequencerBridge::readPattern(int pattern, SequencerStep *steps, int capacity) {
    std::lock_guard<std::mutex> lock(producerLock_);
    if (pattern < 0 || pattern >= kPatterns) return 0;
    // Posted but not yet taken by the audio thread: what was posted is the truth.
    if (appliedGeneration_[pattern].load(std::memory_order_acquire) != postedGeneration_[pattern]) {
        return copyOut(lastPosted_[pattern], steps, capacity);
    }
    const std::atomic<uint32_t> &sequence = publishSequence_[pattern];
    PatternCopy snapshot;
    while (true) {
        const uint32_t before = sequence.load(std::memory_order_acquire);
        if (before & 1) continue;  // mid-write
        snapshot = published_[pattern];
        std::atomic_thread_fence(std::memory_order_acquire);
        if (sequence.load(std::memory_order_relaxed) == before) break;
    }
    return copyOut(snapshot, steps, capacity);
}

int SequencerBridge::copyOut(const PatternCopy &pattern, SequencerStep *steps, int capacity) {
    const int count = std::min(pattern.length, capacity);
    std::copy(pattern.steps.begin(), pattern.steps.begin() + count, steps);
    return count;
}

// MARK: - Audio thread

void SequencerBridge::drain() {
    Command c{};
    while (commands_.pop(c)) {
        switch (c.type) {
            case Command::Settings:
                sequencer_.set(c.settings);
                settings_ = c.settings;
                break;
            case Command::RestOrTie: sequencer_.restOrTie(); break;
            case Command::ClearPattern:
                sequencer_.clearPattern(c.a);
                appliedGeneration_[c.a].store(c.generation, std::memory_order_release);
                break;
            case Command::SetPattern: {
                const PatternCopy &copy = postSlots_[c.b];
                sequencer_.setPattern(c.a, copy.steps.data(), copy.length);
                postSlotBusy_[c.b].store(false, std::memory_order_release);
                appliedGeneration_[c.a].store(c.generation, std::memory_order_release);
                break;
            }
        }
    }
}

int SequencerBridge::advance(int frames, SequencerEvent *out, int capacity) {
    const int count = sequencer_.advance(frames, out, capacity);
    publishChangedPatterns();
    return count;
}

void SequencerBridge::publishChangedPatterns() {
    for (int p = 0; p < kPatterns; ++p) {
        const uint32_t revision = sequencer_.patternRevision(p);
        if (revision == publishedRevision_[p]) continue;
        publishedRevision_[p] = revision;
        std::atomic<uint32_t> &sequence = publishSequence_[p];
        // Odd while writing: a reader that sees an odd or changed number retries.
        sequence.store(sequence.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        PatternCopy &out = published_[p];
        out.length = sequencer_.patternLength(p);
        const SequencerStep *steps = sequencer_.patternSteps(p);
        std::copy(steps, steps + out.length, out.steps.begin());
        sequence.store(sequence.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
}

} // namespace synth
