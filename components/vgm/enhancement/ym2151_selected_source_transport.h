#pragma once

#include "selected_source_transport.h"
#include "ym2151_enhanced_recomposition.h"

#include <cstddef>

namespace gameaudio::vgm {

// YM2151 carries one delivered source lane per complete OPM FM channel. Source
// quality has already been selected before these frames enter the queue; this
// transport only preserves exact identity and output-clock alignment for later
// stereo/Spatial presentation.
using ym2151_selected_source_sample = selected_source_sample;
using ym2151_selected_source_frame =
    selected_source_frame<ym2151_recomposition_source_count>;

template <std::size_t Capacity>
using ym2151_selected_source_queue =
    selected_source_queue<ym2151_recomposition_source_count, Capacity>;

using ym2151_selected_source_block_error = selected_source_block_error;

template <std::size_t MaxFrames = 8192>
using ym2151_selected_source_block_storage =
    selected_source_block_storage<ym2151_recomposition_source_count, MaxFrames>;

} // namespace gameaudio::vgm
