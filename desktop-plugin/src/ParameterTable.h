//
// The synth's parameters as the desktop plugin presents them to a host, and as the
// phone controllers address them over NRPN.
//
// Ranges and defaults copy the Android app's SynthParam.kt, which match the iOS
// app's ParameterDefinitions (min/max). They have to: the iOS Motion Controller
// sends each value as a position across exactly these ranges (ControllerInput.swift),
// and the plugin decodes it back with the same min and max.
//
// The id is the engine's ParamId (Params.h) and the NRPN number, and never changes.
//

#ifndef DRONESYNTH_PLUGIN_PARAMETERTABLE_H
#define DRONESYNTH_PLUGIN_PARAMETERTABLE_H

#include <cstdint>

namespace dronesynth {

enum class ParamKind : uint8_t {
    Linear,
    /// Hosts should draw this one on a logarithmic scale (frequencies, times).
    Log,
    /// Whole numbers only (octave, semitone, delay divisions).
    Integer,
    Boolean,
    /// One of `labels`, stored as its index.
    Choice,
};

struct ParamInfo {
    int id;
    /// Stable symbol (the preset key on Android); hosts and LV2 store patches by it.
    const char *symbol;
    const char *name;
    const char *shortName;
    const char *unit;
    float min;
    float max;
    float def;
    ParamKind kind;
    /// Choice labels, `labelCount` of them; null otherwise.
    const char *const *labels = nullptr;
    int labelCount = 0;
    /// False for values the app derives itself (the reverb's crossover, damping and
    /// decays follow its type and size), which the host never sees but a
    /// controller may still send.
    bool exposed = true;
};

/// Every parameter, exposed ones first in panel order.
const ParamInfo *parameterTable();
int parameterCount();
/// How many of the table's leading entries are exposed to the host.
int exposedParameterCount();
/// The entry for an engine ParamId (= NRPN number), or null.
const ParamInfo *findParameter(int id);

/// iOS's controller "silence everything" command, sent as NRPN 16000.
constexpr int kAllNotesOffNrpn = 16000;
/// One past the highest parameter id, for lookup tables.
constexpr int kMaxParamId = 128;

} // namespace dronesynth

#endif
