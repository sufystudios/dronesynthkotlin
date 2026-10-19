//
// The engine's reverb: the hall (Room, Hall, Cathedral) and the plate. Choosing Plate
// crossfades between them; both run during the crossfade, so the old tail fades out
// while the new one builds. Only the one being heard runs otherwise, and at mix 0
// neither does. Whichever stops is cleared, so it never replays a stale tail when it
// comes back. The Body and Shimmer EQ and the dry/wet mix are applied here, after
// the crossfade.
//

#ifndef DRONESYNTH_REVERBSECTION_H
#define DRONESYNTH_REVERBSECTION_H

#include <vector>

#include "HallReverb.h"
#include "PlateReverb.h"
#include "ReverbBlocks.h"
#include "ReverbSettings.h"

namespace synth {

class ReverbSection {
public:
    /// Allocates everything. Not real-time safe.
    void prepare(double sampleRate);
    void set(const ReverbSettings &settings);
    /// Mixes reverb into the input (dry/wet by the mix setting). The output may alias the input.
    void process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, int frames);

private:
    static constexpr int kChunk = 256;
    static constexpr int kEqStep = 32;  // samples between EQ coefficient updates

    void updateEq();
    /// Runs or idles one reverb for the next chunk, clearing it as it goes idle.
    template <class Reverb>
    void runOrIdle(Reverb &reverb, bool run, bool &running, const float *inLeft, const float *inRight,
                   std::vector<float> &wetLeft, std::vector<float> &wetRight, int frames);

    HallReverb hall_;
    PlateReverb plate_;
    double sampleRate_ = 48000.0;
    ReverbSettings settings_{};
    bool primed_ = false;
    bool hallRunning_ = true, plateRunning_ = true;

    float mix_ = 0.0f, plateAmount_ = 0.0f;
    float mixSmooth_ = 0.0f, typeSmooth_ = 0.0f;
    reverb::Biquad bodyLeft_, bodyRight_, shimmerLeft_, shimmerRight_;
    float eqBody_ = 0.0f, eqShimmer_ = 0.0f;  // dB, gliding toward the settings
    float eqSmooth_ = 0.0f;
    std::vector<float> hallLeft_, hallRight_, plateLeft_, plateRight_;
};

} // namespace synth

#endif //DRONESYNTH_REVERBSECTION_H
