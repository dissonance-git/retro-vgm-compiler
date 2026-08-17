#pragma once

#include "genesis_selected_source_queue.h"

#include <cstddef>

namespace gameaudio::vgm {

// Genesis keeps its public block/error names, but topology validation, ordinal
// delivery, silent-lane elision and fail-closed queue behavior are shared VGM
// transport laws rather than Genesis-specific presentation machinery.
using genesis_selected_source_block_error = selected_source_block_error;

template <std::size_t MaxFrames = 8192>
using genesis_selected_source_block_storage =
    selected_source_block_storage<genesis_recomposition_source_count, MaxFrames>;

} // namespace gameaudio::vgm
