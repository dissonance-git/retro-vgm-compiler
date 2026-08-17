#pragma once

#include "genesis_enhanced_recomposition.h"
#include "selected_source_transport.h"

#include <cstddef>

namespace gameaudio::vgm {

// Preserve the established Genesis transport vocabulary while delegating the
// ordinal/exactness mechanics to the chip-neutral selected-source transport.
using genesis_selected_source_sample = selected_source_sample;
using genesis_selected_source_frame =
    selected_source_frame<genesis_recomposition_source_count>;

template <std::size_t Capacity>
using genesis_selected_source_queue =
    selected_source_queue<genesis_recomposition_source_count, Capacity>;

} // namespace gameaudio::vgm
