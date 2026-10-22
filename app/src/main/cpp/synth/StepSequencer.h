//
// DroneSynth's arpeggiator and step sequencer, modelled on the Moog Grandmother's
// ARP/SEQ module. Shared unchanged between the iOS and Android apps, as the reverb
// is: plain C++17, no platform headers, and no allocation after construction.
//
// It makes no sound itself. Once per render cycle the host calls advance() and gets
// back the note-on/note-off events due in that cycle, each at its exact frame, to
// play on its own voice. Keys and buttons arrive through noteOn/noteOff/restOrTie/
// clearPattern, called on the render thread before advance() (the host moves them
// over from the UI thread). The host forwards every key, all the time: a chord held
// before PLAY is pressed starts the arpeggio the moment it is.
//
// Modes:
//   ARP - cycles through the held notes, then the same again up to two octaves
//         higher (the Grandmother's 0/1/2 octave switch), in one of three orders:
//           Played  - the order the keys went down; a new key joins at the end.
//           UpDown  - lowest to highest and back, repeating the top and bottom
//                     notes where it turns (C E G G E C), so it keeps time in 4/4.
//           Random  - every note once per cycle, reshuffled each cycle.
//         The first key pressed starts it, at once.
//   SEQ - plays the selected pattern. A key press starts it, transposed by that
//         key's distance from the pattern's first recorded note; while it plays,
//         a new key transposes it from the next step.
//   REC - each key press records a step and sounds as it's played. restOrTie()
//         records a rest, or a tie (the previous note held on) while a key is down.
//         Entering REC replaces the pattern only once the first step is recorded,
//         so flipping the switch by accident loses nothing.
// HOLD keeps ARP and SEQ playing once the keys are released; the next key pressed
// after all are up starts a new chord from its first note (ARP), or a new
// transposition (SEQ).
//
// The clock runs whether or not anything plays, so the RATE LED can blink in time
// while the knob turns (clockTicks()). Starting from silence restarts it, so the
// first note never waits for the next tick - unless it follows a host, whose beat
// grid always wins.
//

#ifndef DRONESYNTH_STEPSEQUENCER_H
#define DRONESYNTH_STEPSEQUENCER_H

#include <array>
#include <atomic>
#include <cstdint>

namespace synth {

enum class SequencerMode : int32_t { Arp = 0, Seq = 1, Rec = 2 };
enum class ArpOrder : int32_t { Played = 0, UpDown = 1, Random = 2 };
enum class StepDivision : int32_t {
    Quarter = 0, Eighth = 1, Sixteenth = 2,
    QuarterTriplet = 3, EighthTriplet = 4, SixteenthTriplet = 5,
};

/// Steps per beat (quarter note) for a division.
double stepsPerBeat(StepDivision division);

struct SequencerSettings {
    SequencerMode mode = SequencerMode::Arp;
    bool playing = false;
    bool hold = false;
    double bpm = 120.0;              // 20...300
    StepDivision division = StepDivision::Sixteenth;
    float gate = 0.5f;               // share of each step a note sounds, 0.01...1
    ArpOrder order = ArpOrder::Played;
    int32_t extraOctaves = 0;        // 0...2, octaves played above the held keys
    int32_t pattern = 0;             // 0...kPatterns - 1
};

struct SequencerEvent {
    enum Type : uint8_t { NoteOff = 0, NoteOn = 1 };
    int32_t offset;                  // frame within the cycle
    Type type;
    uint8_t note;
    uint8_t velocity;                // 0 for NoteOff
};

struct SequencerStep {
    enum Kind : uint8_t { Rest = 0, Note = 1, Tie = 2 };
    Kind kind = Rest;
    uint8_t note = 0;
    uint8_t velocity = 0;
};

class StepSequencer {
public:
    static constexpr int kMaxSteps = 256;
    static constexpr int kPatterns = 3;
    static constexpr int kMaxHeld = 16;

    /// Resets everything except the patterns.
    void prepare(double sampleRate);
    void set(const SequencerSettings &settings);

    void noteOn(int note, int velocity);
    void noteOff(int note);
    /// The REC button press: a rest, or a tie while a key is held. Ignored outside REC.
    void restOrTie();
    void clearPattern(int pattern);

    /// Locks steps to the host's beat grid for this cycle: call before advance()
    /// with the beat position at the cycle's first frame. freeRun() lets go.
    void followHost(double beat, double bpm);
    void freeRun();

    /// The events due in the next `frames` frames, in time order. Returns how many
    /// were written; any beyond `capacity` follow at the start of the next cycle.
    int advance(int frames, SequencerEvent *out, int capacity);

    int patternLength(int pattern) const;
    const SequencerStep *patternSteps(int pattern) const;
    /// Replaces a pattern (from a preset). Takes at most kMaxSteps steps.
    void setPattern(int pattern, const SequencerStep *steps, int length);
    void setRandomSeed(uint32_t seed) { random_ = seed ? seed : 1; }
    /// Goes up whenever a pattern's steps change (recorded, cleared or replaced), so
    /// a host can tell when to take a fresh copy. Render thread.
    uint32_t patternRevision(int pattern) const {
        return pattern >= 0 && pattern < kPatterns ? patterns_[pattern].revision : 0;
    }

    /// For the UI, from any thread: the step playing (its index in the arpeggio
    /// or pattern), or -1 when stopped. In REC, the number of steps recorded.
    int currentStep() const { return displayStep_.load(std::memory_order_relaxed); }
    bool isRunning() const { return displayRunning_.load(std::memory_order_relaxed); }
    /// Clock pulses so far, playing or not: the RATE LED flashes when this changes.
    uint32_t clockTicks() const { return displayTicks_.load(std::memory_order_relaxed); }
    /// The gate, for whatever follows it on another thread (the filter envelope):
    /// notes started so far, and whether one is on now.
    uint32_t gateCount() const { return displayGates_.load(std::memory_order_acquire); }
    bool gateOpen() const { return displayGateOpen_.load(std::memory_order_acquire); }

private:
    struct Key { uint8_t note, velocity; };
    struct Pattern {
        std::array<SequencerStep, kMaxSteps> steps{};
        int length = 0;
        uint32_t revision = 0;
    };

    void start();
    void stop();
    void push(SequencerEvent::Type type, int note, int velocity);
    void silenceMonitor();
    void record(SequencerStep step);
    void playStep(int64_t frame, SequencerEvent *out, int &count, int capacity);
    int buildArp();
    void shuffleArp(int length);
    double stepLength() const;
    uint32_t nextRandom();

    static void add(std::array<Key, kMaxHeld> &keys, int &count, int note, int velocity);
    static void remove(std::array<Key, kMaxHeld> &keys, int &count, int note);

    double sampleRate_ = 48000.0;
    SequencerSettings settings_{};
    std::array<Pattern, kPatterns> patterns_{};

    std::array<Key, kMaxHeld> down_{};     // keys physically down, in press order
    int downCount_ = 0;
    std::array<Key, kMaxHeld> held_{};     // what ARP plays: down_, plus latched keys on HOLD
    int heldCount_ = 0;

    bool running_ = false;
    int stepIndex_ = 0;                    // next step of the arpeggio or pattern
    bool reshuffle_ = true;                // Random: shuffle afresh before the next step
    int transposeNote_ = -1;               // SEQ: the key the pattern is played from
    bool recArmed_ = false;
    int monitorNote_ = -1;                 // REC: the key being heard

    int64_t position_ = 0;                 // frames since prepare()
    double nextStepTime_ = 0.0;            // frame of the next step
    int sounding_ = -1;                    // note currently on, or -1
    int64_t pendingOff_ = -1;              // frame its note-off is due, or -1 (tied)

    bool following_ = false;
    double hostBeat_ = 0.0, hostBpm_ = 120.0;
    int64_t hostStep_ = 0;                 // grid step number of the next step
    int64_t lastHostStep_ = INT64_MIN;     // grid step last played

    std::array<SequencerEvent, 2 * kMaxHeld + 8> immediate_{};  // due at the next cycle's start
    int immediateCount_ = 0;
    std::array<Key, kMaxHeld * 3 * 2> arp_{};  // one cycle of the arpeggio, in order
    std::array<Key, kMaxHeld * 3> shuffled_{}; // Random: this cycle's order
    int lastNote_ = -1;                        // Random: kept off the next cycle's first step
    uint32_t random_ = 1;

    std::atomic<int> displayStep_{-1};
    std::atomic<bool> displayRunning_{false};
    std::atomic<uint32_t> displayTicks_{0};
    std::atomic<uint32_t> displayGates_{0};
    std::atomic<bool> displayGateOpen_{false};
};

} // namespace synth

#endif // DRONESYNTH_STEPSEQUENCER_H
