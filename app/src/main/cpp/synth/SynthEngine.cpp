#include "SynthEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

#include "../Logging.h"

namespace synth {

namespace {

/// Left/right filter cutoffs sit this far apart in stereo mode (2.5 cents each way),
/// as in the iOS app's SynthClass.filterStereoDetuneRatio.
const float kStereoDetuneRatio = std::pow(2.0f, 2.5f / 1200.0f);

/// Oscillators are summed at this gain before the envelope and filters.
constexpr float kOscMixGain = 0.9f;

/// Predelay range swept by the Pre-Dly knob, per reverb type (Room, Hall, Plate, Cathedral).
constexpr float kPredelayRange[4][2] = {{10, 40}, {20, 80}, {10, 25}, {40, 100}};

inline bool on(float v) { return v >= 0.5f; }

/// Flushes denormals to zero on the calling thread. Filters, envelopes and reverb
/// tails decaying toward silence otherwise produce denormal numbers, which are very
/// slow on some CPUs and can make the callback miss its deadline (an audible pop).
inline void enableFlushToZero() {
#if defined(__aarch64__)
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    asm volatile("msr fpcr, %0" : : "r"(fpcr | (1ull << 24)));
#elif defined(__arm__)
    uint32_t fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    asm volatile("vmsr fpscr, %0" : : "r"(fpscr | (1u << 24)));
#elif defined(__x86_64__) || defined(__i386__)
    _mm_setcsr(_mm_getcsr() | 0x8040);  // FTZ | DAZ
#endif
}

inline float midiToLogHz(int note) { return float(std::log2(440.0) + (note - 69) / 12.0); }

/// Transparent below 0.9, then a smooth knee so a hot patch can't hard-clip the DAC.
inline float softLimit(float x) {
    const float a = std::fabs(x);
    if (a <= 0.9f) return x;
    const float limited = 0.9f + 0.1f * std::tanh((a - 0.9f) / 0.1f);
    return x < 0.0f ? -limited : limited;
}

} // namespace

SynthEngine::SynthEngine() {
    for (auto &p : params_) p.store(0.0f, std::memory_order_relaxed);
    // Safe starting points until the app pushes its full state.
    params_[kCutoff].store(2000.0f);
    params_[kSustain].store(1.0f);
    params_[kAttack].store(0.1f);
    params_[kDecay].store(0.1f);
    params_[kRelease].store(0.1f);
    params_[kDelayDry].store(1.0f);

    for (auto *eg : {&ampEg_, &filterEg_}) {
        eg->m_bResetToZero = false;  // retrigger from the current level - no clicks
        eg->m_bLegatoMode = false;
    }
    sp_moogladder_create(&filterLeft_);
    sp_moogladder_create(&filterRight_);
}

SynthEngine::~SynthEngine() {
    stop();
    sp_moogladder_destroy(&filterLeft_);
    sp_moogladder_destroy(&filterRight_);
}

// MARK: - Stream

bool SynthEngine::start() {
    std::lock_guard<std::mutex> lock(streamLock_);
    if (stream_ && stream_->getState() == oboe::StreamState::Started) return true;
    if (!stream_ && !openStream()) return false;
    fadeOutRequested_.store(false);
    fadedOut_.store(false);
    const oboe::Result result = stream_->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("Failed to start stream: %s", oboe::convertToText(result));
        return false;
    }
    return true;
}

void SynthEngine::stop() {
    std::lock_guard<std::mutex> lock(streamLock_);
    if (!stream_) return;
    // Fade to silence before stopping, so leaving the app doesn't cut a waveform mid-cycle.
    if (stream_->getState() == oboe::StreamState::Started) {
        fadeOutRequested_.store(true);
        for (int i = 0; i < 20 && !fadedOut_.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    stream_->stop();
    stream_->close();
    stream_.reset();
    latencyTuner_.reset();
}

bool SynthEngine::openStream() {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setFormatConversionAllowed(true)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        ->setChannelConversionAllowed(true)
        ->setUsage(oboe::Usage::Media)
        ->setContentType(oboe::ContentType::Music)
        ->setDataCallback(this)
        ->setErrorCallback(this);
    const oboe::Result result = builder.openStream(stream_);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open stream: %s", oboe::convertToText(result));
        stream_.reset();
        return false;
    }
    // Start double-buffered; the tuner adds a burst each time an underrun happens.
    latencyTuner_ = std::make_unique<oboe::LatencyTuner>(*stream_);
    latencyTuner_->setMinimumBufferSize(stream_->getFramesPerBurst() * 2);
    stream_->setBufferSizeInFrames(stream_->getFramesPerBurst() * 2);
    const double rate = stream_->getSampleRate();
    if (!prepared_ || rate != sampleRate_) prepare(rate);
    LOGI("Stream open: %.0f Hz, burst %d", rate, stream_->getFramesPerBurst());
    return true;
}

void SynthEngine::onErrorAfterClose(oboe::AudioStream *, oboe::Result error) {
    // Oboe calls this on its own thread, e.g. after headphones are unplugged.
    // Reopen on whatever device is now the default.
    LOGW("Stream closed (%s), reopening", oboe::convertToText(error));
    {
        std::lock_guard<std::mutex> lock(streamLock_);
        stream_.reset();
        latencyTuner_.reset();
    }
    start();
}

// MARK: - Preparation (never on the audio thread)

void SynthEngine::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    const float sr = float(sampleRate);
    uint32_t seed = 1;
    for (auto &osc : oscillators_) osc.init(sampleRate, ++seed);
    for (auto *eg : {&ampEg_, &filterEg_}) {
        eg->setSampleRate(sampleRate);
        eg->reset();
    }
    spData_.sr = int(sampleRate);
    spData_.nchan = 2;
    for (auto *f : {filterLeft_, filterRight_}) sp_moogladder_init(&spData_, f);
    filterFreqLeft_ = filterFreqRight_ = 0.0f;  // the first control tick snaps to the knob
    velocity_.setTimeConstant(0.005f, sr);
    velocity_.snap(1.0f);
    stereoAmount_.setTimeConstant(0.02f, sr);
    pitchModOsc12_.setTimeConstant(0.01f, sr);
    pitchModOsc3_.setTimeConstant(0.01f, sr);
    for (auto &p : pitchOffset_) p.setTimeConstant(0.02f / 3.0f, sr);  // ~20 ms to settle
    sustain_.setTimeConstant(0.01f, sr);
    appliedSustain_ = -1.0f;
    masterGain_.setTimeConstant(0.005f, sr);
    masterGain_.snap(0.0f);
    for (auto *s : {&cutoffOctaves_, &resonanceLeft_, &resonanceRight_}) s->setTimeConstant(0.01f, sr);
    ampSmooth_.setTimeConstant(0.001f, sr);
    ampSmooth_.snap(0.0f);
    pitchBend_.setTimeConstant(0.005f, sr);
    modWheel_.setTimeConstant(0.01f, sr);

    delay_.init(sampleRate);
    looper_.prepare(sampleRate);
    recorder_.prepare(sampleRate);

    reverb_.prepare(sampleRate);

    firstApply_ = true;
    prepared_ = true;
}

// MARK: - UI thread API

void SynthEngine::setParam(int id, float value) {
    if (id < 0 || id >= kNumParams || !std::isfinite(value)) return;
    params_[id].store(value, std::memory_order_relaxed);
}

float SynthEngine::getParam(int id) const {
    if (id < 0 || id >= kNumParams) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

void SynthEngine::noteOn(int note, int velocity) {
    std::lock_guard<std::mutex> lock(eventProducerLock_);
    events_.push({NoteEvent::NoteOn, uint8_t(std::clamp(note, 0, 127)), uint8_t(std::clamp(velocity, 1, 127))});
}

void SynthEngine::noteOff(int note) {
    std::lock_guard<std::mutex> lock(eventProducerLock_);
    events_.push({NoteEvent::NoteOff, uint8_t(std::clamp(note, 0, 127)), 0});
}

void SynthEngine::allNotesOff() {
    std::lock_guard<std::mutex> lock(eventProducerLock_);
    events_.push({NoteEvent::AllNotesOff, 0, 0});
}

float SynthEngine::takePeak() {
    return peak_.exchange(0.0f, std::memory_order_relaxed);
}

// MARK: - Notes (audio thread)

void SynthEngine::handleEvent(const NoteEvent &e) {
    switch (e.type) {
        case NoteEvent::NoteOn: {
            // Most-recently-pressed note wins; earlier held notes are remembered so
            // releasing the newest falls back to them without retriggering.
            int w = 0;
            for (int i = 0; i < heldCount_; ++i) {
                if (heldNotes_[i] != e.note) {
                    heldNotes_[w] = heldNotes_[i];
                    heldVelocities_[w] = heldVelocities_[i];
                    ++w;
                }
            }
            heldCount_ = w;
            if (heldCount_ == kMaxHeldNotes) {
                std::move(heldNotes_.begin() + 1, heldNotes_.end(), heldNotes_.begin());
                std::move(heldVelocities_.begin() + 1, heldVelocities_.end(), heldVelocities_.begin());
                --heldCount_;
            }
            heldNotes_[heldCount_] = e.note;
            heldVelocities_[heldCount_] = e.velocity;
            ++heldCount_;
            triggerNote(e.note, e.velocity, true);
            break;
        }
        case NoteEvent::NoteOff: {
            int w = 0;
            for (int i = 0; i < heldCount_; ++i) {
                if (heldNotes_[i] != e.note) {
                    heldNotes_[w] = heldNotes_[i];
                    heldVelocities_[w] = heldVelocities_[i];
                    ++w;
                }
            }
            const bool removed = w != heldCount_;
            heldCount_ = w;
            if (!removed) break;
            if (heldCount_ == 0) {
                releaseNote();
            } else {
                triggerNote(heldNotes_[heldCount_ - 1], heldVelocities_[heldCount_ - 1], false);
            }
            break;
        }
        case NoteEvent::AllNotesOff:
            heldCount_ = 0;
            releaseNote();
            break;
    }
}

void SynthEngine::triggerNote(int note, int velocity, bool retrigger) {
    velocity_.setTarget(float(velocity) / 127.0f);
    targetLogHz_ = midiToLogHz(note);
    // Portamento: slide from wherever the pitch is now, over the glide time. Even
    // with glide at 0 the slide takes 4 ms, too short to hear as portamento but
    // enough to avoid the click of an instant pitch jump.
    const double glide = std::max(double(glideTime_), 0.004);
    glideStep_ = std::fabs(targetLogHz_ - currentLogHz_) / std::max(1.0, glide * sampleRate_);
    if (!retrigger) return;
    ampEg_.startEG();
    filterEg_.startEG();
    gateOpen_ = true;
}

void SynthEngine::releaseNote() {
    // In drone mode the gate stays open after the keys are lifted.
    if (droneMode_ || !gateOpen_) return;
    ampEg_.noteOff();
    filterEg_.noteOff();
    gateOpen_ = false;
}

// MARK: - Parameters (audio thread)

void SynthEngine::applyParams(const Snapshot &s) {
    const auto &p = s.v;
    auto changed = [&](int id) { return firstApply_ || p[id] != applied_.v[id]; };
    auto envMs = [](float seconds) { return std::max(2.0, double(seconds) * 1000.0); };

    for (int k = 0; k < 3; ++k) {
        const int base = kOsc1Waveform + k * 8;
        OscillatorSlot &osc = oscillators_[k];
        osc.setWaveform(Waveform(std::clamp(int(std::lround(p[base + kOscWaveformOffset])), 0, 4)));
        osc.volume.setTarget(std::clamp(p[base + kOscVolumeOffset], 0.0f, 1.0f));
        pitchOffset_[k].setTarget(std::round(p[base + kOscOctaveOffset]) * 1200.0f +
                                  std::round(p[base + kOscSemitoneOffset]) * 100.0f +
                                  p[base + kOscFineOffset]);
        if (firstApply_) {
            osc.volume.snap(osc.volume.target());
            pitchOffset_[k].snap(pitchOffset_[k].target());
        }
        DriftingLfo &vib = vibrato_[k];
        vib.rate = std::clamp(p[base + kOscVibratoRateOffset], 0.05f, 20.0f);
        vib.randomize = 0.0f;
    }
    syncOsc3_ = on(p[kOsc3SyncToOsc1]);

    if (changed(kAttack)) { ampEg_.setAttackTime_mSec(envMs(p[kAttack])); filterEg_.setAttackTime_mSec(envMs(p[kAttack])); }
    if (changed(kDecay)) { ampEg_.setDecayTime_mSec(envMs(p[kDecay])); filterEg_.setDecayTime_mSec(envMs(p[kDecay])); }
    if (changed(kRelease)) { ampEg_.setReleaseTime_mSec(envMs(p[kRelease])); filterEg_.setReleaseTime_mSec(envMs(p[kRelease])); }
    sustain_.setTarget(std::clamp(p[kSustain], 0.0f, 1.0f));
    if (firstApply_) sustain_.snap(sustain_.target());
    glideTime_ = std::clamp(p[kGlideTime], 0.0f, 2.0f);

    const bool drone = on(p[kDroneMode]);
    if (drone != droneMode_) {
        droneMode_ = drone;
        // Switching drone off behaves exactly like lifting every key.
        if (!drone) {
            heldCount_ = 0;
            releaseNote();
        }
    }

    for (auto [lfo, base] : {std::pair{&filterLfo_, int(kFilterLfoShape)},
                             std::pair{&pitchLfoOsc12_, int(kPitchLfoOsc12Shape)},
                             std::pair{&pitchLfoOsc3_, int(kPitchLfoOsc3Shape)}}) {
        lfo->shape = LfoShape(std::clamp(int(std::lround(p[base + kLfoShapeOffset])), 0, 3));
        lfo->rate = std::clamp(p[base + kLfoRateOffset], 0.05f, 20.0f);
        lfo->randomize = std::clamp(p[base + kLfoRandomizeOffset], 0.0f, 1.0f);
    }

    stereoAmount_.setTarget(on(p[kStereoFiltersEnabled]) ? 1.0f : 0.0f);
    cutoffOctaves_.setTarget(std::log2(std::clamp(p[kCutoff], 20.0f, 20000.0f)));
    resonanceLeft_.setTarget(std::clamp(p[kLeftResonance], 0.0f, 0.99f));
    resonanceRight_.setTarget(std::clamp(p[kRightResonance], 0.0f, 0.99f));
    // General MIDI's default bend range: ±2 semitones.
    pitchBend_.setTarget(std::clamp(p[kPitchBend], -1.0f, 1.0f) * 200.0f);
    modWheel_.setTarget(std::clamp(p[kModWheel], 0.0f, 1.0f));

    delay_.time = p[kDelayTime];
    delay_.subdivide = p[kDelaySubdivide];
    delay_.timeLeft = p[kDelayTimeLeft];
    delay_.timeRight = p[kDelayTimeRight];
    delay_.mixMode = std::clamp(int(std::lround(p[kDelayMixMode])), 0, 3);
    delay_.medium = std::clamp(int(std::lround(p[kDelayMedium])), 0, 2);
    delay_.setFeedback(p[kDelayFeedback]);
    delay_.setAmount(p[kDelayWet]);
    delay_.setDry(p[kDelayDry]);
    delay_.setWidth(p[kDelayWidth]);
    if (firstApply_) delay_.snapSmoothedValues();

    ReverbSettings reverb;
    const int type = std::clamp(int(std::lround(p[kReverbType])), 0, 3);
    reverb.type = ReverbType(type);
    reverb.size = std::clamp(p[kReverbSize], 0.0f, 1.0f);
    const float predelay = std::clamp(p[kReverbPredelay], 0.0f, 1.0f);
    reverb.predelayMs = kPredelayRange[type][0] + predelay * (kPredelayRange[type][1] - kPredelayRange[type][0]);
    reverb.crossoverHz = std::clamp(p[kReverbLowCutoff], 50.0f, 1000.0f);
    reverb.rt60Low = std::clamp(p[kReverbLowDecay], 1.0f, 8.0f);
    reverb.rt60Mid = std::clamp(p[kReverbHighDecay], 1.0f, 8.0f);
    reverb.dampingHz = std::clamp(p[kReverbDamping], 1500.0f, 20000.0f);
    reverb.bodyDb = std::clamp(p[kReverbBody], -15.0f, 15.0f);
    reverb.shimmerDb = std::clamp(p[kReverbShimmer], -15.0f, 15.0f);
    reverb.mix = std::clamp(p[kReverbMix], 0.0f, 0.9f);
    // The reverb glides to new settings itself, so they can be handed over directly.
    reverb_.set(reverb);

    if (firstApply_) {
        stereoAmount_.snap(stereoAmount_.target());
        for (auto *s : {&cutoffOctaves_, &resonanceLeft_, &resonanceRight_}) s->snap(s->target());
    }
    applied_ = s;
    firstApply_ = false;
}

// MARK: - Control rate

void SynthEngine::controlTick(const Snapshot &s, int frames) {
    const auto &p = s.v;
    const float dt = float(frames / sampleRate_);

    const float sustain = sustain_.advance(frames);
    if (std::fabs(sustain - appliedSustain_) > 1e-5f) {
        appliedSustain_ = sustain;
        ampEg_.setSustainLevel(sustain);
        filterEg_.setSustainLevel(sustain);
    }

    // Every LFO keeps running even while switched off, so it stays smooth when re-enabled.
    const float filterMod = filterLfo_.tick(dt);
    const float osc12Mod = pitchLfoOsc12_.tick(dt);
    const float osc3Mod = pitchLfoOsc3_.tick(dt);

    // Pitch LFOs only move while a note is sounding (gate open, including drone).
    if (gateOpen_) {
        const float osc12Depth = on(p[kPitchLfoOsc12Shape + kLfoEnabledOffset]) ? p[kPitchLfoOsc12Shape + kLfoDepthOffset] : 0.0f;
        const float osc3Depth = on(p[kPitchLfoOsc3Shape + kLfoEnabledOffset]) ? p[kPitchLfoOsc3Shape + kLfoDepthOffset] : 0.0f;
        pitchModOsc12_.setTarget(osc12Depth * 200.0f * osc12Mod);  // cents
        // OSC 3's LFO matches its ±700 cent Pitch knob, for wide hard-sync sweeps.
        pitchModOsc3_.setTarget(osc3Depth * 700.0f * osc3Mod);
    }
    const float bendCents = pitchBend_.advance(frames);
    const float osc12Cents = pitchModOsc12_.advance(frames) + bendCents;
    const float osc3Cents = pitchModOsc3_.advance(frames) + bendCents;

    // Glide toward the target note in the log-frequency domain.
    if (currentLogHz_ != targetLogHz_) {
        const double step = glideStep_ * frames;
        if (step <= 0.0 || std::fabs(targetLogHz_ - currentLogHz_) <= step) {
            currentLogHz_ = targetLogHz_;
        } else {
            currentLogHz_ += targetLogHz_ > currentLogHz_ ? step : -step;
        }
    }

    for (int k = 0; k < 3; ++k) {
        const int base = kOsc1Waveform + k * 8;
        float cents = pitchOffset_[k].advance(frames);
        cents += k < 2 ? osc12Cents : osc3Cents;
        const float vib = vibrato_[k].tick(dt);
        if (on(p[base + kOscVibratoEnabledOffset])) cents += p[base + kOscVibratoDepthOffset] * 700.0f * vib;
        oscillators_[k].setFrequency(std::exp2(currentLogHz_ + cents / 1200.0));
    }

    // Filter cutoff: knob × filter LFO (±3 octaves) × filter envelope (up to +6 octaves).
    float octaves = 0.0f;
    if (on(p[kFilterLfoShape + kLfoEnabledOffset])) octaves += p[kFilterLfoShape + kLfoDepthOffset] * 3.0f * filterMod;
    if (on(p[kFilterEnvModEnabled])) octaves += p[kFilterEnvModAmount] * 6.0f * filterEnvLevel_;
    // The mod wheel adds up to 8 kHz on top, as on iOS (MIDI CC1).
    const float modBoost = modWheel_.advance(frames) * 8000.0f;
    const float cutoff = std::clamp(std::exp2(cutoffOctaves_.advance(frames) + octaves) + modBoost, 20.0f, 20000.0f);
    const float spread = std::pow(kStereoDetuneRatio, stereoAmount_.value());
    // Resonance 0...0.99 goes straight to moogladder, as iOS passes it to MoogLadder.
    filterLeft_->res = resonanceLeft_.advance(frames);
    filterRight_->res = resonanceRight_.advance(frames);
    // render() ramps the cutoff across the control block a sample at a time, as
    // AudioKit's parameter ramper does on iOS: stepping it every 8 samples clicks.
    const float targetLeft = cutoff / spread;
    const float targetRight = cutoff * spread;
    if (filterFreqLeft_ <= 0.0f) {
        filterFreqLeft_ = targetLeft;
        filterFreqRight_ = targetRight;
    }
    filterStepLeft_ = (targetLeft - filterFreqLeft_) / float(frames);
    filterStepRight_ = (targetRight - filterFreqRight_) / float(frames);
}

// MARK: - Audio

void SynthEngine::render(float *left, float *right, int frames) {
    const float stereo = stereoAmount_.advance(frames);
    for (int i = 0; i < frames; ++i) {
        const float o1 = oscillators_[0].render();
        const float o2 = oscillators_[1].render();
        // Hard sync: OSC 3 restarts its cycle whenever OSC 1 starts a new one.
        if (syncOsc3_ && oscillators_[0].wrapped()) oscillators_[2].hardSync(oscillators_[0].samplesSinceWrap());
        const float o3 = oscillators_[2].render();
        const float mix = o1 * oscillators_[0].volume.next() +
                          o2 * oscillators_[1].volume.next() +
                          o3 * oscillators_[2].volume.next();

        ampSmooth_.setTarget(float(ampEg_.doEnvelope()));
        const float amp = ampSmooth_.next();
        filterEnvLevel_ = float(filterEg_.doEnvelope());
        const float x = mix * kOscMixGain * amp * velocity_.next();

        filterLeft_->freq = filterFreqLeft_ += filterStepLeft_;
        filterRight_->freq = filterFreqRight_ += filterStepRight_;
        float in = x, l, r;
        sp_moogladder_compute(&spData_, filterLeft_, &in, &l);
        sp_moogladder_compute(&spData_, filterRight_, &in, &r);
        left[i] = l;
        right[i] = l + stereo * (r - l);
    }
}

oboe::DataCallbackResult SynthEngine::onAudioReady(oboe::AudioStream *, void *audioData, int32_t numFrames) {
    enableFlushToZero();
    if (latencyTuner_) latencyTuner_->tune();
    auto *out = static_cast<float *>(audioData);
    for (int i = 0; i < kNumParams; ++i) snapshot_.v[i] = params_[i].load(std::memory_order_relaxed);
    applyParams(snapshot_);
    NoteEvent event{};
    while (events_.pop(event)) handleEvent(event);

    float peak = 0.0f;
    int done = 0;
    while (done < numFrames) {
        const int chunk = std::min(numFrames - done, kMaxBlock);
        for (int offset = 0; offset < chunk; offset += kControlBlock) {
            const int n = std::min(kControlBlock, chunk - offset);
            controlTick(snapshot_, n);
            render(left_.data() + offset, right_.data() + offset, n);
        }

        delay_.process(left_.data(), right_.data(), chunk);

        // The reverb mixes dry and wet internally. It always runs, even at mix 0: skipping
        // it and resuming later would replay a stale tail.
        reverb_.process(left_.data(), right_.data(), wetLeft_.data(), wetRight_.data(), chunk);

        // The looper records the synth's output and mixes its playback back in.
        looper_.process(wetLeft_.data(), wetRight_.data(), wetLeft_.data(), wetRight_.data(), chunk);

        masterGain_.setTarget(fadeOutRequested_.load(std::memory_order_relaxed) ? 0.0f : 1.0f);
        for (int i = 0; i < chunk; ++i) {
            const float gain = masterGain_.next();
            const float l = softLimit(wetLeft_[i]) * gain;
            const float r = softLimit(wetRight_[i]) * gain;
            peak = std::max(peak, std::max(std::fabs(l), std::fabs(r)));
            out[2 * (done + i)] = l;
            out[2 * (done + i) + 1] = r;
            left_[i] = l;
            right_[i] = r;
        }
        // The recorder takes the final output, loop included.
        recorder_.write(left_.data(), right_.data(), chunk);
        done += chunk;
    }

    if (fadeOutRequested_.load(std::memory_order_relaxed) && masterGain_.value() < 1e-4f) {
        fadedOut_.store(true, std::memory_order_relaxed);
    }

    float previous = peak_.load(std::memory_order_relaxed);
    while (peak > previous && !peak_.compare_exchange_weak(previous, peak, std::memory_order_relaxed)) {}
    return oboe::DataCallbackResult::Continue;
}

} // namespace synth
