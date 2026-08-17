#pragma once

#include "vgm_realtime_musical_omniphony_pipeline.h"
#include "ym2151_enhanced_recomposition.h"

#include <cstddef>

namespace gameaudio::vgm {

using ym2151_realtime_musical_omniphony_result =
    vgm_realtime_musical_omniphony_result;

template <
    std::size_t MaxFrames = 8192,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
using ym2151_realtime_musical_omniphony_pipeline =
    vgm_realtime_musical_omniphony_pipeline<
        ym2151_recomposition_source_count,
        MaxFrames,
        MaxEvents,
        RoleCapacity>;

} // namespace gameaudio::vgm
