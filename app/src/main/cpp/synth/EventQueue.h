//
// Single-producer / single-consumer lock-free queue for note events, written
// from the UI thread and drained on the audio thread at the start of each
// render callback. Never allocates or blocks after construction.
//

#ifndef DRONESYNTH_EVENTQUEUE_H
#define DRONESYNTH_EVENTQUEUE_H

#include <array>
#include <atomic>
#include <cstdint>

namespace synth {

struct NoteEvent {
    enum Type : uint8_t { NoteOn, NoteOff, AllNotesOff };
    Type type;
    uint8_t note;
    uint8_t velocity;
};

template <typename T, size_t Capacity>
class SpscQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    /// Producer side. Returns false (dropping the event) if the queue is full.
    bool push(const T &item) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity) return false;
        items_[head & (Capacity - 1)] = item;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    /// Consumer side.
    bool pop(T &item) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);
        if (tail == head) return false;
        item = items_[tail & (Capacity - 1)];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

private:
    std::array<T, Capacity> items_{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace synth

#endif //DRONESYNTH_EVENTQUEUE_H
