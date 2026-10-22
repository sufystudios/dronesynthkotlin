#include "StepSequencer.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace synth {

double stepsPerBeat(StepDivision division) {
    switch (division) {
        case StepDivision::Quarter: return 1.0;
        case StepDivision::Eighth: return 2.0;
        case StepDivision::Sixteenth: return 4.0;
        case StepDivision::QuarterTriplet: return 1.5;
        case StepDivision::EighthTriplet: return 3.0;
        case StepDivision::SixteenthTriplet: return 6.0;
    }
    return 4.0;
}

// MARK: - Setup and settings

void StepSequencer::prepare(double sampleRate) {
    sampleRate_ = sampleRate > 0 ? sampleRate : 48000.0;
    downCount_ = heldCount_ = 0;
    running_ = false;
    stepIndex_ = 0;
    reshuffle_ = true;
    lastNote_ = -1;
    transposeNote_ = -1;
    recArmed_ = settings_.mode == SequencerMode::Rec;
    monitorNote_ = -1;
    position_ = 0;
    nextStepTime_ = 0.0;
    sounding_ = -1;
    pendingOff_ = -1;
    following_ = false;
    lastHostStep_ = INT64_MIN;
    immediateCount_ = 0;
    displayGateOpen_.store(false, std::memory_order_release);
    displayStep_.store(-1, std::memory_order_relaxed);
    displayRunning_.store(false, std::memory_order_relaxed);
}

void StepSequencer::set(const SequencerSettings &settings) {
    SequencerSettings next = settings;
    next.mode = SequencerMode(std::clamp(int32_t(next.mode), 0, 2));
    next.order = ArpOrder(std::clamp(int32_t(next.order), 0, 2));
    next.division = StepDivision(std::clamp(int32_t(next.division), 0, 5));
    next.bpm = std::clamp(next.bpm, 20.0, 300.0);
    next.gate = std::clamp(next.gate, 0.01f, 1.0f);
    next.extraOctaves = std::clamp(next.extraOctaves, 0, 2);
    next.pattern = std::clamp(next.pattern, 0, kPatterns - 1);

    const SequencerSettings previous = settings_;
    settings_ = next;

    const bool modeChanged = previous.mode != next.mode;
    if (modeChanged || previous.playing != next.playing) {
        stop();
        silenceMonitor();
    }
    if (modeChanged) {
        // REC doesn't latch; whatever's down is what's held in the new mode.
        held_ = down_;
        heldCount_ = downCount_;
    }
    if (next.mode == SequencerMode::Rec && (modeChanged || previous.pattern != next.pattern)) {
        // Armed: the pattern is replaced only once the first new step comes in.
        recArmed_ = true;
        displayStep_.store(0, std::memory_order_relaxed);
    }
    if (previous.hold && !next.hold) {
        // Letting go of HOLD drops the latched keys that aren't still down.
        held_ = down_;
        heldCount_ = downCount_;
        reshuffle_ = true;
        if (running_ && (next.mode == SequencerMode::Arp ? heldCount_ == 0 : downCount_ == 0)) stop();
    }
    if (previous.order != next.order || previous.extraOctaves != next.extraOctaves) reshuffle_ = true;

    // PLAY pressed with keys already down starts at once.
    if (!running_ && next.playing && next.mode != SequencerMode::Rec) {
        if (next.mode == SequencerMode::Arp && heldCount_ > 0) {
            start();
        } else if (next.mode == SequencerMode::Seq && downCount_ > 0) {
            transposeNote_ = down_[downCount_ - 1].note;
            start();
        }
    }
}

// MARK: - Keys and buttons

void StepSequencer::noteOn(int note, int velocity) {
    if (note < 0 || note > 127) return;
    velocity = std::clamp(velocity, 1, 127);
    const bool allWereUp = downCount_ == 0;
    add(down_, downCount_, note, velocity);

    if (settings_.mode == SequencerMode::Rec) {
        record({SequencerStep::Note, uint8_t(note), uint8_t(velocity)});
        silenceMonitor();
        push(SequencerEvent::NoteOn, note, velocity);
        monitorNote_ = note;
        return;
    }

    if (settings_.hold && allWereUp) {
        // A new chord on HOLD replaces the latched one, from its first note. (In SEQ
        // the key only transposes: the pattern carries on where it is.)
        heldCount_ = 0;
        if (settings_.mode == SequencerMode::Arp) stepIndex_ = 0;
    }
    add(held_, heldCount_, note, velocity);
    reshuffle_ = true;

    if (!settings_.playing) return;
    if (settings_.mode == SequencerMode::Seq) transposeNote_ = note;
    if (!running_) start();
}

void StepSequencer::noteOff(int note) {
    remove(down_, downCount_, note);
    if (settings_.mode == SequencerMode::Rec) {
        if (note == monitorNote_) silenceMonitor();
        return;
    }
    if (!settings_.hold) {
        remove(held_, heldCount_, note);
        reshuffle_ = true;
    }
    if (!running_) return;
    if (settings_.mode == SequencerMode::Arp) {
        if (heldCount_ == 0) stop();
    } else if (downCount_ > 0) {
        transposeNote_ = down_[downCount_ - 1].note;
    } else if (!settings_.hold) {
        stop();
    }
}

void StepSequencer::restOrTie() {
    if (settings_.mode != SequencerMode::Rec) return;
    record({downCount_ > 0 ? SequencerStep::Tie : SequencerStep::Rest, 0, 0});
}

void StepSequencer::clearPattern(int pattern) {
    if (pattern < 0 || pattern >= kPatterns) return;
    patterns_[pattern].length = 0;
    ++patterns_[pattern].revision;
    if (running_ && settings_.mode == SequencerMode::Seq && settings_.pattern == pattern) stop();
}

void StepSequencer::record(SequencerStep step) {
    Pattern &p = patterns_[settings_.pattern];
    if (recArmed_) {
        p.length = 0;
        recArmed_ = false;
    }
    if (p.length < kMaxSteps) p.steps[p.length++] = step;
    ++p.revision;
    displayStep_.store(p.length, std::memory_order_relaxed);
}

// MARK: - Host sync

void StepSequencer::followHost(double beat, double bpm) {
    following_ = true;
    hostBeat_ = beat;
    hostBpm_ = std::clamp(bpm, 1.0, 999.0);
}

void StepSequencer::freeRun() {
    following_ = false;
}

double StepSequencer::stepLength() const {
    const double bpm = following_ ? hostBpm_ : settings_.bpm;
    return sampleRate_ * 60.0 / bpm / stepsPerBeat(settings_.division);
}

// MARK: - Running

void StepSequencer::start() {
    running_ = true;
    stepIndex_ = 0;
    reshuffle_ = true;
    lastNote_ = -1;
    // Restart the clock so the first note plays now (this runs before the cycle's
    // advance, at its first frame). A host's grid isn't ours to move.
    if (!following_) nextStepTime_ = double(position_);
    displayRunning_.store(true, std::memory_order_relaxed);
}

void StepSequencer::stop() {
    if (sounding_ >= 0) push(SequencerEvent::NoteOff, sounding_, 0);
    sounding_ = -1;
    pendingOff_ = -1;
    running_ = false;
    displayRunning_.store(false, std::memory_order_relaxed);
    if (settings_.mode != SequencerMode::Rec) displayStep_.store(-1, std::memory_order_relaxed);
}

void StepSequencer::push(SequencerEvent::Type type, int note, int velocity) {
    if (immediateCount_ < int(immediate_.size()))
        immediate_[immediateCount_++] = {0, type, uint8_t(note), uint8_t(velocity)};
}

void StepSequencer::silenceMonitor() {
    if (monitorNote_ >= 0) push(SequencerEvent::NoteOff, monitorNote_, 0);
    monitorNote_ = -1;
}

int StepSequencer::advance(int frames, SequencerEvent *out, int capacity) {
    int count = 0;
    int sent = 0;
    while (sent < immediateCount_ && count < capacity) out[count++] = immediate_[sent++];
    std::copy(immediate_.begin() + sent, immediate_.begin() + immediateCount_, immediate_.begin());
    immediateCount_ -= sent;
    if (frames <= 0) return count;

    const int64_t end = position_ + frames;
    const double length = stepLength();
    if (following_) {
        // The next grid step at or after this cycle's start, never one already played.
        const double steps = hostBeat_ * stepsPerBeat(settings_.division);
        int64_t step = int64_t(std::ceil(steps - 1e-6));
        if (step <= lastHostStep_) step = lastHostStep_ + 1;
        hostStep_ = step;
        nextStepTime_ = double(position_) + std::max(0.0, (double(step) - steps) * length);
    }

    // Two slots per pass: a step can end one note and start the next.
    while (count + 2 <= capacity) {
        // Anything late (held back by a full buffer) plays at the cycle's start.
        const int64_t on = std::max<int64_t>(std::llround(nextStepTime_), position_);
        const int64_t off = pendingOff_ >= 0 ? std::max(pendingOff_, position_) : INT64_MAX;
        if (off <= on && off < end) {
            // Closing first when both fall on one frame lets a full-length gate retrigger.
            out[count++] = {int32_t(off - position_), SequencerEvent::NoteOff, uint8_t(sounding_), 0};
            sounding_ = -1;
            pendingOff_ = -1;
        } else if (on < end) {
            displayTicks_.fetch_add(1, std::memory_order_relaxed);
            if (following_) lastHostStep_ = hostStep_++;
            nextStepTime_ += length;
            if (running_) playStep(on, out, count, capacity);
        } else {
            break;
        }
    }
    position_ = end;

    for (int i = 0; i < count; ++i) {
        if (out[i].type == SequencerEvent::NoteOn) {
            displayGates_.fetch_add(1, std::memory_order_release);
            displayGateOpen_.store(true, std::memory_order_release);
        } else {
            displayGateOpen_.store(false, std::memory_order_release);
        }
    }
    return count;
}

void StepSequencer::playStep(int64_t frame, SequencerEvent *out, int &count, int capacity) {
    int note = -1, velocity = 0, index = 0;
    bool tie = false, nextIsTie = false;

    if (settings_.mode == SequencerMode::Arp) {
        const int length = buildArp();
        if (length == 0) return;
        index = stepIndex_ % length;
        stepIndex_ = index + 1;
        Key key = arp_[index];
        if (settings_.order == ArpOrder::Random) {
            if (index == 0 || reshuffle_) shuffleArp(length);
            key = shuffled_[index];
        }
        note = key.note;
        velocity = key.velocity;
    } else {
        const Pattern &p = patterns_[settings_.pattern];
        if (p.length == 0) return;
        index = stepIndex_ % p.length;
        stepIndex_ = index + 1;
        const SequencerStep &step = p.steps[index];
        nextIsTie = p.length > 1 && p.steps[(index + 1) % p.length].kind == SequencerStep::Tie;
        if (step.kind == SequencerStep::Tie) {
            tie = true;
        } else if (step.kind == SequencerStep::Note) {
            // Transposed by the key's distance from the pattern's first note.
            int root = step.note;
            for (int i = 0; i < p.length; ++i) {
                if (p.steps[i].kind == SequencerStep::Note) {
                    root = p.steps[i].note;
                    break;
                }
            }
            const int transposed = step.note + (transposeNote_ >= 0 ? transposeNote_ - root : 0);
            if (transposed >= 0 && transposed <= 127) {
                note = transposed;
                velocity = step.velocity;
            }
        }
    }
    displayStep_.store(index, std::memory_order_relaxed);

    // The gate ends a share of the step in, but never after the next step starts.
    const int64_t nextOn = std::llround(nextStepTime_);
    const int64_t gateOff = std::min(frame + std::max<int64_t>(1, std::llround(settings_.gate * stepLength())), nextOn);

    if (tie) {
        // Holds the previous note on; after a rest there's nothing to hold.
        if (sounding_ >= 0) pendingOff_ = nextIsTie ? -1 : gateOff;
        return;
    }
    if (note < 0) return;  // a rest
    if (sounding_ >= 0 && count < capacity) {
        // Still on from a tie: end it as the new note starts.
        out[count++] = {int32_t(frame - position_), SequencerEvent::NoteOff, uint8_t(sounding_), 0};
    }
    if (count < capacity) out[count++] = {int32_t(frame - position_), SequencerEvent::NoteOn, uint8_t(note), uint8_t(velocity)};
    sounding_ = note;
    pendingOff_ = nextIsTie ? -1 : gateOff;
    lastNote_ = note;
}

// MARK: - The arpeggio

int StepSequencer::buildArp() {
    std::array<Key, kMaxHeld> keys = held_;
    const int n = heldCount_;
    if (settings_.order != ArpOrder::Played) {
        std::stable_sort(keys.begin(), keys.begin() + n, [](Key a, Key b) { return a.note < b.note; });
    }
    // The held keys, then the same again an octave up, per extra octave.
    int m = 0;
    for (int octave = 0; octave <= settings_.extraOctaves; ++octave) {
        for (int i = 0; i < n; ++i) {
            const int note = keys[i].note + 12 * octave;
            if (note <= 127) arp_[m++] = {uint8_t(note), keys[i].velocity};
        }
    }
    if (settings_.order == ArpOrder::UpDown) {
        // Up, then back down repeating the top and bottom notes: C E G G E C.
        std::stable_sort(arp_.begin(), arp_.begin() + m, [](Key a, Key b) { return a.note < b.note; });
        for (int i = 0; i < m; ++i) arp_[m + i] = arp_[m - 1 - i];
        return 2 * m;
    }
    return m;
}

void StepSequencer::shuffleArp(int length) {
    std::copy(arp_.begin(), arp_.begin() + length, shuffled_.begin());
    for (int i = length - 1; i > 0; --i) std::swap(shuffled_[i], shuffled_[nextRandom() % uint32_t(i + 1)]);
    // Never the note just played again across the seam between cycles.
    if (length > 1 && shuffled_[0].note == lastNote_) {
        std::swap(shuffled_[0], shuffled_[1 + nextRandom() % uint32_t(length - 1)]);
    }
    reshuffle_ = false;
}

uint32_t StepSequencer::nextRandom() {
    // xorshift32: cheap, allocation-free, and repeatable from a seed for tests.
    random_ ^= random_ << 13;
    random_ ^= random_ >> 17;
    random_ ^= random_ << 5;
    return random_;
}

// MARK: - Key lists and patterns

void StepSequencer::add(std::array<Key, kMaxHeld> &keys, int &count, int note, int velocity) {
    for (int i = 0; i < count; ++i) {
        if (keys[i].note == note) {
            keys[i].velocity = uint8_t(velocity);  // pressed again: keeps its place
            return;
        }
    }
    if (count < kMaxHeld) keys[count++] = {uint8_t(note), uint8_t(velocity)};
}

void StepSequencer::remove(std::array<Key, kMaxHeld> &keys, int &count, int note) {
    for (int i = 0; i < count; ++i) {
        if (keys[i].note == note) {
            std::copy(keys.begin() + i + 1, keys.begin() + count, keys.begin() + i);
            --count;
            return;
        }
    }
}

int StepSequencer::patternLength(int pattern) const {
    return pattern >= 0 && pattern < kPatterns ? patterns_[pattern].length : 0;
}

const SequencerStep *StepSequencer::patternSteps(int pattern) const {
    return pattern >= 0 && pattern < kPatterns ? patterns_[pattern].steps.data() : nullptr;
}

void StepSequencer::setPattern(int pattern, const SequencerStep *steps, int length) {
    if (pattern < 0 || pattern >= kPatterns) return;
    Pattern &p = patterns_[pattern];
    p.length = steps ? std::clamp(length, 0, kMaxSteps) : 0;
    std::copy(steps, steps + p.length, p.steps.begin());
    ++p.revision;
}

} // namespace synth
