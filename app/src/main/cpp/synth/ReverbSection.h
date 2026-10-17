//
// The engine's reverb: the hall (Room, Hall, Cathedral) and the plate both run all
// the time, and choosing Plate crossfades between them - each already has its own
// tail going, so switching never cuts one off or replays a stale one. The Body and
// Shimmer EQ and the dry/wet mix are applied here, after the crossfade.
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

    HallReverb hall_;
    PlateReverb plate_;
    double sampleRate_ = 48000.0;
    ReverbSettings settings_{};
    bool primed_ = false;

    float mix_ = 0.0f, plateAmount_ = 0.0f;
    float mixSmooth_ = 0.0f, typeSmooth_ = 0.0f;
    reverb::Biquad bodyLeft_, bodyRight_, shimmerLeft_, shimmerRight_;
    float eqBody_ = 0.0f, eqShimmer_ = 0.0f;  // dB, gliding toward the settings
    float eqSmooth_ = 0.0f;
    std::vector<float> hallLeft_, hallRight_, plateLeft_, plateRight_;
};

} // namespace synth

#endif //DRONESYNTH_REVERBSECTION_H
