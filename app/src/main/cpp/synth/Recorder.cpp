#include "Recorder.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace synth {

void Recorder::prepare(double sampleRate) {
    sampleRate_ = int(sampleRate);
    maxFrames_ = int64_t(kMaxSeconds * sampleRate);
}

bool Recorder::start(const std::string &path) {
    stop();
    file_ = std::fopen(path.c_str(), "wb");
    if (!file_) return false;
    dataBytes_ = 0;
    writeHeader(0);  // placeholder sizes, patched in stop()
    head_.store(0);
    tail_.store(0);
    frames_.store(0);
    writerRunning_.store(true);
    writer_ = std::thread(&Recorder::writerLoop, this);
    recording_.store(true, std::memory_order_release);
    return true;
}

void Recorder::stop() {
    recording_.store(false, std::memory_order_release);
    if (!writer_.joinable()) return;
    writerRunning_.store(false);
    writer_.join();
    drain();
    writeHeader(uint32_t(std::min<uint64_t>(dataBytes_, 0xFFFFFFF0u)));
    std::fclose(file_);
    file_ = nullptr;
}

// MARK: - Audio thread

void Recorder::write(const float *left, const float *right, int frames) {
    if (!recording_.load(std::memory_order_acquire)) return;
    const size_t head = head_.load(std::memory_order_relaxed);
    const size_t tail = tail_.load(std::memory_order_acquire);
    int64_t recorded = frames_.load(std::memory_order_relaxed);
    const int64_t room = std::min<int64_t>(int64_t(kRingSamples - (head - tail)) / 2, maxFrames_ - recorded);
    const int n = int(std::min<int64_t>(frames, room));
    for (int i = 0; i < n; ++i) {
        ring_[(head + 2 * size_t(i)) & (kRingSamples - 1)] = int16_t(std::lrint(std::clamp(left[i], -1.0f, 1.0f) * 32767.0f));
        ring_[(head + 2 * size_t(i) + 1) & (kRingSamples - 1)] = int16_t(std::lrint(std::clamp(right[i], -1.0f, 1.0f) * 32767.0f));
    }
    head_.store(head + 2 * size_t(n), std::memory_order_release);
    recorded += n;
    frames_.store(recorded, std::memory_order_relaxed);
    // At the 3 minute limit the take stops itself; the UI finalises the file.
    if (recorded >= maxFrames_) recording_.store(false, std::memory_order_release);
}

// MARK: - Writer thread

void Recorder::writerLoop() {
    while (writerRunning_.load()) {
        drain();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

size_t Recorder::drain() {
    const size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_relaxed);
    const size_t available = head - tail;
    size_t remaining = available;
    while (remaining > 0) {
        const size_t index = tail & (kRingSamples - 1);
        const size_t chunk = std::min(remaining, kRingSamples - index);
        std::fwrite(ring_.data() + index, sizeof(int16_t), chunk, file_);
        tail += chunk;
        remaining -= chunk;
    }
    tail_.store(tail, std::memory_order_release);
    dataBytes_ += available * sizeof(int16_t);
    return available;
}

void Recorder::writeHeader(uint32_t dataBytes) {
    auto u32 = [this](uint32_t v) { std::fwrite(&v, 4, 1, file_); };
    auto u16 = [this](uint16_t v) { std::fwrite(&v, 2, 1, file_); };
    const uint16_t channels = 2, bits = 16;
    std::fseek(file_, 0, SEEK_SET);
    std::fwrite("RIFF", 1, 4, file_);
    u32(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, file_);
    u32(16);
    u16(1);  // PCM
    u16(channels);
    u32(uint32_t(sampleRate_));
    u32(uint32_t(sampleRate_) * channels * bits / 8);
    u16(channels * bits / 8);
    u16(bits);
    std::fwrite("data", 1, 4, file_);
    u32(dataBytes);
    std::fseek(file_, 0, SEEK_END);
}

} // namespace synth
