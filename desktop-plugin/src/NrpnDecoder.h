//
// Decodes the Drone Synth controller protocol: each parameter is an NRPN whose
// number is the parameter's id, with 14-bit data spread linearly across the
// parameter's range. A C++ copy of the receiving half of the iOS app's
// ControllerInput.swift, so the iPhone's Motion Controller (and the Android one, once
// it exists) drive this plugin exactly as they drive the AUv3:
//
//   CC 99 (NRPN MSB), CC 98 (NRPN LSB), CC 6 (data MSB), CC 38 (data LSB)
//
// The value is applied on the data LSB. NRPN 16000 means "silence everything".
// Allocation- and lock-free: it runs on the audio thread.
//

#ifndef DRONESYNTH_PLUGIN_NRPNDECODER_H
#define DRONESYNTH_PLUGIN_NRPNDECODER_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "ParameterTable.h"

namespace dronesynth {

class NrpnDecoder {
public:
    struct Result {
        enum Type { None, SetParameter, AllNotesOff } type = None;
        int id = -1;
        float value = 0.0f;
    };

    static constexpr int kMaxData = 16383;

    NrpnDecoder() {
        selected_.fill(-1);
        numberMSB_.fill(0);
        pendingMSB_.fill(-1);
    }

    /// One control change. Returns what, if anything, it completed.
    Result receive(uint8_t controller, uint8_t value, uint8_t channel) {
        const int ch = channel & 0x0F;
        const int v = value & 0x7F;
        Result result;
        switch (controller) {
            case 99:
                numberMSB_[ch] = v;
                selected_[ch] = -1;
                pendingMSB_[ch] = -1;
                break;
            case 98:
                selected_[ch] = (numberMSB_[ch] << 7) | v;
                pendingMSB_[ch] = -1;
                break;
            case 101:
            case 100:
                // An RPN shares the data CCs: what follows is its data, not ours.
                selected_[ch] = -1;
                pendingMSB_[ch] = -1;
                break;
            case 6:
                pendingMSB_[ch] = v;
                break;
            case 38: {
                const int number = selected_[ch];
                const int msb = pendingMSB_[ch];
                pendingMSB_[ch] = -1;
                if (msb < 0 || number < 0) break;
                if (number == kAllNotesOffNrpn) {
                    result.type = Result::AllNotesOff;
                    break;
                }
                const ParamInfo *info = number < kMaxParamId ? findParameter(number) : nullptr;
                if (!info) break;
                const float position = float((msb << 7) | v) / float(kMaxData);
                float decoded = info->min + (info->max - info->min) * position;
                // iOS rounds only its indexed and boolean parameters.
                if (info->kind == ParamKind::Choice || info->kind == ParamKind::Boolean) decoded = std::round(decoded);
                result.type = Result::SetParameter;
                result.id = number;
                result.value = decoded;
                break;
            }
            default:
                break;
        }
        return result;
    }

    /// The four control changes that set `id` to `value`, as the phone sends them.
    /// For tests, and for the phone side of the protocol.
    static std::array<std::array<uint8_t, 3>, 4> encode(int id, float value, uint8_t channel = 0) {
        int data = 0;
        if (const ParamInfo *info = findParameter(id); info && info->max > info->min) {
            const float clamped = std::min(std::max(value, info->min), info->max);
            data = int(std::lround((clamped - info->min) / (info->max - info->min) * kMaxData));
        }
        const uint8_t status = uint8_t(0xB0 | (channel & 0x0F));
        return {{
            {status, 99, uint8_t((id >> 7) & 0x7F)},
            {status, 98, uint8_t(id & 0x7F)},
            {status, 6, uint8_t((data >> 7) & 0x7F)},
            {status, 38, uint8_t(data & 0x7F)},
        }};
    }

private:
    std::array<int, 16> selected_;
    std::array<int, 16> numberMSB_;
    std::array<int, 16> pendingMSB_;
};

} // namespace dronesynth

#endif
