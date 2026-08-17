#pragma once

#include "genesis_enhanced_recomposition.h"
#include "spatial_source_bus.h"

#include <cstddef>

namespace gameaudio::vgm {

using genesis_spatial_source_bus_error = spatial_source_bus_error;

template <std::size_t MaxFrames = 8192>
using genesis_spatial_source_bus_storage =
    spatial_source_bus_storage<genesis_recomposition_source_count, MaxFrames>;

} // namespace gameaudio::vgm
