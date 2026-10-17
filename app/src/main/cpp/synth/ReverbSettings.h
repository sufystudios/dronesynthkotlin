//
// The reverb controls, shared by the hall and plate algorithms. They are the Zita
// reverb's controls (which the Reverb drawer and presets were built around).
//

#ifndef DRONESYNTH_REVERBSETTINGS_H
#define DRONESYNTH_REVERBSETTINGS_H

namespace synth {

enum class ReverbType : int { Room = 0, Hall = 1, Plate = 2, Cathedral = 3 };

struct ReverbSettings {
    ReverbType type = ReverbType::Hall;
    float size = 0.5f;          // 0...1, scales the space's dimensions
    float predelayMs = 50.0f;
    float crossoverHz = 200.0f; // below this, rt60Low applies
    float rt60Low = 3.0f;       // seconds
    float rt60Mid = 3.0f;       // seconds
    float dampingHz = 16000.0f; // where the mid decay time halves
    float bodyDb = 0.0f;        // EQ at 250 Hz on the wet signal
    float shimmerDb = 0.0f;     // EQ at 6 kHz on the wet signal
    float mix = 0.0f;           // 0 = dry ... 1 = wet
};

} // namespace synth

#endif //DRONESYNTH_REVERBSETTINGS_H
