//
// The Drone Synth engine: a monophonic three-oscillator synth with stereo Moog
// ladder filters, a tube stage, the Vintage Delay and a Zita reverb, rendered
// through Oboe. Mirrors the iOS app's SynthClass signal chain:
//
//   OSC 1/2/3 ─► mix ─► amp EG × velocity ─► Moog L/R ─► tube ─► delay ─► reverb ─► out
//
// Threading: the UI thread only ever writes parameters (atomics) and pushes note
// events (a lock-free queue). Everything else happens on the audio thread.
//

#ifndef DRONESYNTH_SYNTHENGINE_H
#define DRONESYNTH_SYNTHENGINE_H

#include <oboe/Oboe.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>

#include "../EnvelopeGenerator.h"
#include "../MoogLadderFilter.h"
#include "../vendor/soundpipe/Soundpipe.h"
#include "EventQueue.h"
#include "Modulators.h"
#include "OscillatorSlot.h"
#include "Params.h"
#include "VintageDelay.h"

namespace synth {

class SynthEngine : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
public:
    SynthEngine();
    ~SynthEngine() override;

    /// Opens and starts the output stream. Safe to call repeatedly.
    bool start();
    /// Stops and closes the output stream.
    void stop();

    void setParam(int id, float value);
    float getParam(int id) const;
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void allNotesOff();

    /// Peak output level since the last call (for the UI's meter/visuals), 0...1+.
    float takePeak();
    double sampleRate() const { return sampleRate_; }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *stream, void *audioData, int32_t numFrames) override;
    void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override;

private:
    static constexpr int kControlBlock = 16;
    static constexpr int kMaxBlock = 1024;
    static constexpr int kMaxHeldNotes = 16;

    /// A snapshot of every parameter, taken once per render callback.
    struct Snapshot { std::array<float, kNumParams> v; };

    bool openStream();
    void prepare(double sampleRate);
    void applyParams(const Snapshot &p);
    void handleEvent(const NoteEvent &e);
    void triggerNote(int note, int velocity, bool retrigger);
    void releaseNote();
    void controlTick(const Snapshot &p, int frames);
    void render(float *left, float *right, int frames);

    // --- stream
    std::mutex streamLock_;
    std::shared_ptr<oboe::AudioStream> stream_;
    double sampleRate_ = 48000.0;
    bool prepared_ = false;

    // --- UI -> audio
    std::array<std::atomic<float>, kNumParams> params_;
    SpscQueue<NoteEvent, 256> events_;
    std::atomic<float> peak_{0.0f};

    // --- voice
    std::array<OscillatorSlot, 3> oscillators_;
    std::array<DriftingLfo, 3> vibrato_{DriftingLfo(11), DriftingLfo(12), DriftingLfo(13)};
    DriftingLfo filterLfo_{21};
    DriftingLfo pitchLfoOsc12_{22};
    DriftingLfo pitchLfoOsc3_{23};
    CEnvelopeGenerator ampEg_;
    CEnvelopeGenerator filterEg_;
    CMoogLadderFilter filterLeft_;
    CMoogLadderFilter filterRight_;

    Smoothed velocity_;
    Smoothed stereoAmount_;
    Smoothed tubeAmount_;
    Smoothed pitchModOsc12_;
    Smoothed pitchModOsc3_;
    float tubeDrive_ = 3.0f;
    float filterEnvLevel_ = 0.0f;
    bool syncOsc3_ = false;

    // --- notes
    std::array<uint8_t, kMaxHeldNotes> heldNotes_{};
    std::array<uint8_t, kMaxHeldNotes> heldVelocities_{};
    int heldCount_ = 0;
    bool gateOpen_ = false;
    bool droneMode_ = false;
    double currentLogHz_ = std::log2(110.0);
    double targetLogHz_ = std::log2(110.0);
    double glideStep_ = 0.0;
    float glideTime_ = 0.1f;

    // --- effects
    VintageDelay delay_;
    sp_data spData_{};
    sp_zitarev *reverb_ = nullptr;
    Smoothed reverbMix_;

    // --- scratch buffers (allocated in prepare)
    std::array<float, kMaxBlock> left_{};
    std::array<float, kMaxBlock> right_{};
    std::array<float, kMaxBlock> wetLeft_{};
    std::array<float, kMaxBlock> wetRight_{};
    Snapshot snapshot_{};
    Snapshot applied_{};
    bool firstApply_ = true;
};

} // namespace synth

#endif //DRONESYNTH_SYNTHENGINE_H
