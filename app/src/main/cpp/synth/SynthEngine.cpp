#include "SynthEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "../Logging.h"

namespace synth {

namespace {

/// Left/right filter cutoffs sit this far apart in stereo mode (2.5 cents each way),
/// as in the iOS app's SynthClass.filterStereoDetuneRatio.
const float kStereoDetuneRatio = std::pow(2.0f, 2.5f / 1200.0f);

/// Oscillators are summed at this gain before the envelope and filters.
constexpr float kOscMixGain = 0.9f;

/// The reverb's fixed EQ bands, as on iOS (only the levels are exposed).
constexpr float kReverbBodyFrequency = 250.0f;
constexpr float kReverbShimmerFrequency = 6000.0f;

/// Predelay range swept by the Pre-Dly knob, per reverb type (Room, Hall, Plate, Cathedral).
constexpr float kPredelayRange[4][2] = {{10, 40}, {20, 80}, {10, 25}, {40, 100}};

inline bool on(float v) { return v >= 0.5f; }

inline float midiToLogHz(int note) { return float(std::log2(440.0) + (note - 69) / 12.0); }

/// Tube stage: the old app's atan waveshaper. Scaled for a gentle +3 dB at low levels
/// (rather than the raw drive/atan(drive) boost, up to ~14 dB) so more drive adds
/// saturation and compression instead of just volume.
inline float tube(float x, float drive) { return 1.414f * std::atan(drive * x) / drive; }

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
    params_[kTubeDrive].store(3.0f);

    for (auto *eg : {&ampEg_, &filterEg_}) {
        eg->m_bResetToZero = false;  // retrigger from the current level - no clicks
        eg->m_bLegatoMode = false;
    }
    for (auto *f : {&filterLeft_, &filterRight_}) {
        f->m_uNLP = CFilter::ON;  // tanh in the ladder's feedback path, like the real thing
        f->m_dSaturation = 1.5;
    }
}

SynthEngine::~SynthEngine() {
    stop();
    if (reverb_) sp_zitarev_destroy(&reverb_);
}

// MARK: - Stream

bool SynthEngine::start() {
    std::lock_guard<std::mutex> lock(streamLock_);
    if (stream_ && stream_->getState() == oboe::StreamState::Started) return true;
    if (!stream_ && !openStream()) return false;
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
    stream_->stop();
    stream_->close();
    stream_.reset();
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
    // Double buffering keeps latency low while leaving headroom against underruns.
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
    for (auto *f : {&filterLeft_, &filterRight_}) {
        f->setSampleRate(sampleRate);
        f->reset();
    }
    velocity_.setTimeConstant(0.005f, sr);
    velocity_.snap(1.0f);
    stereoAmount_.setTimeConstant(0.02f, sr);
    tubeAmount_.setTimeConstant(0.02f, sr);
    pitchModOsc12_.setTimeConstant(0.01f, sr);
    pitchModOsc3_.setTimeConstant(0.01f, sr);
    reverbMix_.setTimeConstant(0.03f, sr);

    delay_.init(sampleRate);

    if (reverb_) sp_zitarev_destroy(&reverb_);
    spData_.sr = int(sampleRate);
    spData_.nchan = 2;
    sp_zitarev_create(&reverb_);
    sp_zitarev_init(&spData_, reverb_);
    *reverb_->eq1_freq = kReverbBodyFrequency;
    *reverb_->eq2_freq = kReverbShimmerFrequency;
    *reverb_->level = 0.0f;

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
    events_.push({NoteEvent::NoteOn, uint8_t(std::clamp(note, 0, 127)), uint8_t(std::clamp(velocity, 1, 127))});
}

void SynthEngine::noteOff(int note) {
    events_.push({NoteEvent::NoteOff, uint8_t(std::clamp(note, 0, 127)), 0});
}

void SynthEngine::allNotesOff() {
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
    // Portamento: slide from wherever the pitch is now, over the glide time.
    const double frames = std::max(1.0, double(glideTime_) * sampleRate_);
    glideStep_ = glideTime_ > 0.0005f ? std::fabs(targetLogHz_ - currentLogHz_) / frames : 0.0;
    if (glideStep_ == 0.0) currentLogHz_ = targetLogHz_;
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
        if (firstApply_) osc.volume.snap(osc.volume.target());
        DriftingLfo &vib = vibrato_[k];
        vib.rate = std::clamp(p[base + kOscVibratoRateOffset], 0.05f, 20.0f);
        vib.randomize = 0.0f;
    }
    syncOsc3_ = on(p[kOsc3SyncToOsc1]);

    if (changed(kAttack)) { ampEg_.setAttackTime_mSec(envMs(p[kAttack])); filterEg_.setAttackTime_mSec(envMs(p[kAttack])); }
    if (changed(kDecay)) { ampEg_.setDecayTime_mSec(envMs(p[kDecay])); filterEg_.setDecayTime_mSec(envMs(p[kDecay])); }
    if (changed(kRelease)) { ampEg_.setReleaseTime_mSec(envMs(p[kRelease])); filterEg_.setReleaseTime_mSec(envMs(p[kRelease])); }
    if (changed(kSustain)) {
        const double sustain = std::clamp(double(p[kSustain]), 0.0, 1.0);
        ampEg_.setSustainLevel(sustain);
        filterEg_.setSustainLevel(sustain);
    }
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
    tubeAmount_.setTarget(on(p[kTubeEnabled]) ? 1.0f : 0.0f);
    tubeDrive_ = std::clamp(p[kTubeDrive], 1.0f, 10.0f);
    for (auto [filter, res] : {std::pair{&filterLeft_, p[kLeftResonance]},
                               std::pair{&filterRight_, p[kRightResonance]}}) {
        // Resonance 0...0.99 maps onto the ladder's Q control 1...10 (feedback K 0...4).
        filter->m_dQControl = 1.0 + 9.0 * std::clamp(double(res), 0.0, 0.99);
    }

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

    const int type = std::clamp(int(std::lround(p[kReverbType])), 0, 3);
    const float predelay = std::clamp(p[kReverbPredelay], 0.0f, 1.0f);
    *reverb_->in_delay = kPredelayRange[type][0] + predelay * (kPredelayRange[type][1] - kPredelayRange[type][0]);
    *reverb_->lf_x = std::clamp(p[kReverbLowCutoff], 50.0f, 1000.0f);
    *reverb_->rt60_low = std::clamp(p[kReverbLowDecay], 1.0f, 8.0f);
    *reverb_->rt60_mid = std::clamp(p[kReverbHighDecay], 1.0f, 8.0f);
    *reverb_->hf_damping = std::clamp(p[kReverbDamping], 1500.0f, 20000.0f);
    *reverb_->eq1_level = std::clamp(p[kReverbBody], -15.0f, 15.0f);
    *reverb_->eq2_level = std::clamp(p[kReverbShimmer], -15.0f, 15.0f);
    reverbMix_.setTarget(std::clamp(p[kReverbMix], 0.0f, 0.9f));

    if (firstApply_) {
        stereoAmount_.snap(stereoAmount_.target());
        tubeAmount_.snap(tubeAmount_.target());
        reverbMix_.snap(reverbMix_.target());
    }
    applied_ = s;
    firstApply_ = false;
}

// MARK: - Control rate

void SynthEngine::controlTick(const Snapshot &s, int frames) {
    const auto &p = s.v;
    const float dt = float(frames / sampleRate_);

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
    const float osc12Cents = pitchModOsc12_.advance(frames);
    const float osc3Cents = pitchModOsc3_.advance(frames);

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
        float cents = std::round(p[base + kOscOctaveOffset]) * 1200.0f +
                      std::round(p[base + kOscSemitoneOffset]) * 100.0f +
                      p[base + kOscFineOffset];
        cents += k < 2 ? osc12Cents : osc3Cents;
        const float vib = vibrato_[k].tick(dt);
        if (on(p[base + kOscVibratoEnabledOffset])) cents += p[base + kOscVibratoDepthOffset] * 700.0f * vib;
        oscillators_[k].setFrequency(std::exp2(currentLogHz_ + cents / 1200.0));
    }

    // Filter cutoff: knob × filter LFO (±3 octaves) × filter envelope (up to +6 octaves).
    float octaves = 0.0f;
    if (on(p[kFilterLfoShape + kLfoEnabledOffset])) octaves += p[kFilterLfoShape + kLfoDepthOffset] * 3.0f * filterMod;
    if (on(p[kFilterEnvModEnabled])) octaves += p[kFilterEnvModAmount] * 6.0f * filterEnvLevel_;
    const float cutoff = std::clamp(p[kCutoff] * std::exp2(octaves), 20.0f, 20000.0f);
    const float spread = std::pow(kStereoDetuneRatio, stereoAmount_.value());
    filterLeft_.m_dFcControl = cutoff / spread;
    filterRight_.m_dFcControl = cutoff * spread;
    filterLeft_.update();
    filterRight_.update();
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

        const float amp = float(ampEg_.doEnvelope());
        filterEnvLevel_ = float(filterEg_.doEnvelope());
        const float x = mix * kOscMixGain * amp * velocity_.next();

        const float l = float(filterLeft_.doFilter(x));
        const float r = float(filterRight_.doFilter(x));
        float outL = l;
        float outR = l + stereo * (r - l);

        const float t = tubeAmount_.next();
        if (t > 0.0001f) {
            outL += t * (tube(outL, tubeDrive_) - outL);
            outR += t * (tube(outR, tubeDrive_) - outR);
        }
        left[i] = outL;
        right[i] = outR;
    }
}

oboe::DataCallbackResult SynthEngine::onAudioReady(oboe::AudioStream *, void *audioData, int32_t numFrames) {
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

        // Zita mixes dry and wet internally; skip it entirely while it's silent.
        const float mixStart = reverbMix_.value();
        const float mixEnd = reverbMix_.advance(chunk);
        if (mixStart > 0.0001f || mixEnd > 0.0001f) {
            *reverb_->mix = mixEnd;
            sp_zitarev_compute_block(reverb_, chunk, left_.data(), right_.data(), wetLeft_.data(), wetRight_.data());
            std::memcpy(left_.data(), wetLeft_.data(), sizeof(float) * chunk);
            std::memcpy(right_.data(), wetRight_.data(), sizeof(float) * chunk);
        }

        for (int i = 0; i < chunk; ++i) {
            const float l = softLimit(left_[i]);
            const float r = softLimit(right_[i]);
            peak = std::max(peak, std::max(std::fabs(l), std::fabs(r)));
            out[2 * (done + i)] = l;
            out[2 * (done + i) + 1] = r;
        }
        done += chunk;
    }

    float previous = peak_.load(std::memory_order_relaxed);
    while (peak > previous && !peak_.compare_exchange_weak(previous, peak, std::memory_order_relaxed)) {}
    return oboe::DataCallbackResult::Continue;
}

} // namespace synth
