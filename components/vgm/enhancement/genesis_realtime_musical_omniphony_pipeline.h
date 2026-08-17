#pragma once

#include "genesis_enhanced_recomposition.h"
#include "vgm_realtime_musical_omniphony_pipeline.h"

#include <cstddef>

namespace gameaudio::vgm {

using genesis_realtime_musical_omniphony_result =
    vgm_realtime_musical_omniphony_result;

template <
    std::size_t MaxFrames = 8192,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
using genesis_realtime_musical_omniphony_pipeline =
    vgm_realtime_musical_omniphony_pipeline<
        genesis_recomposition_source_count,
        MaxFrames,
        MaxEvents,
        RoleCapacity>;

} // namespace gameaudio::vgm
