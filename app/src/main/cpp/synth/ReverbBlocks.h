//
// Small building blocks shared by the reverbs: a fractional delay line, allpass
// diffusers, one-pole filters and a peaking EQ.
//

#ifndef DRONESYNTH_REVERBBLOCKS_H
#define DRONESYNTH_REVERBBLOCKS_H

#include <algorithm>
#include <cmath>
#include <vector>

namespace synth::reverb {

constexpr float kTwoPi = 6.28318530718f;

/// The one-pole lowpass coefficient for a cutoff of `hz`.
inline float onePoleCoeff(float hz, double sampleRate) {
    return 1.0f - std::exp(-kTwoPi * std::min(hz, float(0.45 * sampleRate)) / float(sampleRate));
}

/// A delay line of power-of-two length, read at whole or fractional positions.
struct Delay {
    std::vector<float> buffer;
    int mask = 0;
    int write = 0;

    void allocate(int minimumLength) {
        int size = 1;
        while (size < minimumLength + 4) size <<= 1;
        buffer.assign(size, 0.0f);
        mask = size - 1;
        write = 0;
    }
    void clear() { std::fill(buffer.begin(), buffer.end(), 0.0f); write = 0; }
    void push(float x) { buffer[write] = x; write = (write + 1) & mask; }
    /// The sample pushed `delay` pushes before the most recent one.
    float readInt(int delay) const { return buffer[(write - 1 - delay) & mask]; }
    /// Reads `delay` samples back with cubic (Hermite) interpolation.
    float read(float delay) const {
        delay = std::max(delay, 1.0f);
        const int i = int(delay);
        const float f = delay - float(i);
        const float xm1 = readInt(i - 1), x0 = readInt(i), x1 = readInt(i + 1), x2 = readInt(i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

    /// read() with its position and interpolation weights worked out in advance,
    /// for a delay that holds still across many samples.
    struct Tap {
        int delay = 1;
        float w[4] = {0.0f, 1.0f, 0.0f, 0.0f};  // for the samples at delay - 1 ... delay + 2

        void set(float samples) {
            samples = std::max(samples, 1.0f);
            delay = int(samples);
            const float f = samples - float(delay), f2 = f * f, f3 = f2 * f;
            // The Hermite polynomial of read(), regrouped per sample.
            w[0] = -0.5f * f + f2 - 0.5f * f3;
            w[1] = 1.0f - 2.5f * f2 + 1.5f * f3;
            w[2] = 0.5f * f + 2.0f * f2 - 1.5f * f3;
            w[3] = -0.5f * f2 + 0.5f * f3;
        }
    };
    float read(const Tap &tap) const {
        const int base = write - 1 - tap.delay;
        return tap.w[0] * buffer[(base + 1) & mask] + tap.w[1] * buffer[base & mask] +
               tap.w[2] * buffer[(base - 1) & mask] + tap.w[3] * buffer[(base - 2) & mask];
    }
};

/// Schroeder allpass. `process` uses a fixed length; `processAt` a fractional,
/// possibly moving one (up to the allocated length).
struct Allpass {
    Delay delay;
    int length = 1;

    void allocate(int len, int maximum = 0) {
        length = std::max(1, len);
        delay.allocate(std::max(length, maximum));
    }
    void clear() { delay.clear(); }
    float process(float x, float g) { return run(x, g, delay.readInt(length - 1)); }
    float processAt(float x, float g, float len) { return run(x, g, delay.read(len - 1.0f)); }

private:
    float run(float x, float g, float delayed) {
        const float w = x - g * delayed;
        delay.push(w);
        return delayed + g * w;
    }
};

struct OnePole {
    float z = 0.0f;
    float lowpass(float x, float a) { z += a * (x - z); return z; }
};

/// RBJ peaking EQ.
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setPeaking(float hz, float q, float db, float sampleRate) {
        const float a = std::pow(10.0f, db / 40.0f);
        const float w0 = kTwoPi * hz / sampleRate;
        const float alpha = std::sin(w0) / (2.0f * q);
        const float cosw = std::cos(w0);
        const float a0 = 1.0f + alpha / a;
        b0 = (1.0f + alpha * a) / a0;
        b1 = -2.0f * cosw / a0;
        b2 = (1.0f - alpha * a) / a0;
        a1 = -2.0f * cosw / a0;
        a2 = (1.0f - alpha / a) / a0;
    }
    void clear() { z1 = z2 = 0.0f; }
    float process(float x) {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

/// One band-split decay stage: the low band (below the crossover) loses `gainLow`
/// per pass and the rest `gainMid`; above the damping frequency the mid gain applies
/// twice, so the decay time halves there.
struct DecayStage {
    OnePole crossover, damping;
    float process(float x, float gainLow, float gainMid, float crossoverCoeff, float dampingCoeff) {
        const float low = crossover.lowpass(x, crossoverCoeff);
        float y = gainMid * (x - low) + gainLow * low;
        const float high = y - damping.lowpass(y, dampingCoeff);
        return y - (1.0f - gainMid) * high;
    }
};

} // namespace synth::reverb

#endif //DRONESYNTH_REVERBBLOCKS_H
