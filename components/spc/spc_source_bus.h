#pragma once

#include "spc_runtime_capture.h"
#include "spc_spatial_source.h"
#include "../../model/spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Source-native S-DSP handoff shape.
//
// The eight voices are tapped after interpolation/noise selection and envelope
// application, before the signed VxVOLL/VxVOLR multipliers. The echo return is
// one shared stereo feedback field, represented as two linked mono lanes so no
// L/R information is discarded to satisfy a mono source-render ABI.
class spc_source_bus {
public:
    static constexpr std::size_t voice_count = 8;
    static constexpr std::size_t echo_lane_count = 2;
    static constexpr std::size_t lane_count = voice_count + echo_lane_count;

    enum class echo_side : std::uint8_t {
        left = 0,
        right = 1,
    };

    struct frame_view {
        std::array<const float*, voice_count> dry_voice{};
        const float* echo_left = nullptr;
        const float* echo_right = nullptr;
        std::size_t frame_count = 0;
    };

    // A stable field identity links the left/right return lanes. It is not a
    // musical-part id and must never be confused with one of the eight voices.
    static constexpr std::uint64_t echo_field_id(std::uint32_t generation) noexcept {
        return 0x5344535045434800ULL | static_cast<std::uint64_t>(generation);
    }

    static constexpr std::uint64_t echo_lane_id(
        echo_side side,
        std::uint32_t generation) noexcept {
        return echo_field_id(generation)
            ^ (side == echo_side::left ? 0x4cULL : 0x52ULL);
    }

    static constexpr float normalize_echo_volume(std::int8_t gain) noexcept {
        return gain == -128 ? -1.0f : static_cast<float>(gain) / 127.0f;
    }

    static vgmtooling::model::spatial_source_evidence make_echo_source(
        echo_side side,
        std::uint32_t generation,
        std::int8_t echo_volume) noexcept {
        vgmtooling::model::spatial_source_evidence source;
        source.source_id = echo_lane_id(side, generation);
        source.generation = generation;
        source.family = vgmtooling::model::spatial_source_family::spc;

        // Link the stereo halves into one persistent environmental field. The
        // two lanes retain hard side authority plus signed EVOL polarity.
        source.persistent_part_present = true;
        source.persistent_part_id = echo_field_id(generation);
        source.stereo_route.present = true;
        source.stereo_route.authority =
            vgmtooling::model::spatial_evidence_authority::device_authored;
        if (side == echo_side::left) {
            source.stereo_route.left_gain = normalize_echo_volume(echo_volume);
            source.stereo_route.right_gain = 0.0f;
        } else {
            source.stereo_route.left_gain = 0.0f;
            source.stereo_route.right_gain = normalize_echo_volume(echo_volume);
        }

        // The return is known to be a shared field. These are presentation
        // facts, not authored 3-D coordinates.
        source.presentation.diffuse = 1.0f;
        source.presentation.width = 1.0f;
        source.presentation.confidence = 1.0f;
        return source;
    }
};

} // namespace gameaudio::spc
