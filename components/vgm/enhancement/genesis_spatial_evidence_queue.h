#pragma once

#include "genesis_enhanced_recomposition.h"
#include "spatial_route_transport.h"

#include <cstddef>

namespace gameaudio::vgm {

// Genesis keeps its established evidence-queue vocabulary while the sparse
// ordinal sideband is shared by every VGM source family.
using genesis_spatial_evidence_transition = spatial_evidence_transition;

template <std::size_t Capacity>
using genesis_spatial_evidence_queue =
    spatial_evidence_queue<genesis_recomposition_source_count, Capacity>;

} // namespace gameaudio::vgm
