//
// The Recorder (the iOS app's REC drawer): captures up to 3 minutes of the final
// output to a 16-bit stereo WAV file. The audio thread only copies samples into a
// lock-free ring buffer; a writer thread drains it to disk, so file I/O can never
// stall the audio.
//

#ifndef DRONESYNTH_RECORDER_H
#define DRONESYNTH_RECORDER_H

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace synth {

class Recorder {
public:
    static constexpr double kMaxSeconds = 180.0;

    ~Recorder() { stop(); }

    void prepare(double sampleRate);

    /// UI thread. Starts a new take at `path` (overwriting it). False if the file can't be opened.
    bool start(const std::string &path);
    /// UI thread. Stops recording, flushes and finalises the WAV file. Safe to call at any time.
    void stop();

    /// Audio thread. Appends a block of the final output while recording.
    void write(const float *left, const float *right, int frames);

    bool isRecording() const { return recording_.load(std::memory_order_relaxed); }
    /// True while a take is open: recording, or auto-stopped at the limit but not yet finalised.
    bool isOpen() const { return writer_.joinable(); }
    int64_t recordedFrames() const { return frames_.load(std::memory_order_relaxed); }

private:
    void writerLoop();
    size_t drain();
    void writeHeader(uint32_t dataBytes);

    static constexpr size_t kRingSamples = size_t(1) << 19;  // ~2.7 s of stereo at 48 kHz
    std::vector<int16_t> ring_ = std::vector<int16_t>(kRingSamples);
    std::atomic<size_t> head_{0};  // written by the audio thread
    std::atomic<size_t> tail_{0};  // written by the writer thread

    std::atomic<bool> recording_{false};
    std::atomic<bool> writerRunning_{false};
    std::atomic<int64_t> frames_{0};
    int64_t maxFrames_ = 0;
    int sampleRate_ = 48000;

    std::thread writer_;
    FILE *file_ = nullptr;
    uint64_t dataBytes_ = 0;
};

} // namespace synth

#endif //DRONESYNTH_RECORDER_H
