#pragma once

#include "genesis_enhanced_recomposition.h"
#include "timed_spatial_source_bus.h"

#include <cstddef>

namespace gameaudio::vgm {

using genesis_timed_spatial_source_bus_error = timed_spatial_source_bus_error;

template <std::size_t MaxFrames = 8192, std::size_t MaxEvents = 256>
using genesis_timed_spatial_source_bus_storage =
    timed_spatial_source_bus_storage<
        genesis_recomposition_source_count,
        MaxFrames,
        MaxEvents>;

} // namespace gameaudio::vgm
