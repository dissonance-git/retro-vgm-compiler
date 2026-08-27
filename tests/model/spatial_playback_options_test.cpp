#include "../../model/spatial_playback_options.h"

#include <cassert>

using namespace vgmtooling::model;

int main() {
    spatial_playback_options reference{};
    assert(resolve_spatial_playback(reference) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(reference));

    auto surround = reference;
    surround.surround = true;
    assert(resolve_spatial_playback(surround) == spatial_playback_path::source_spatial);
    assert(uses_source_renderer(surround));
    return 0;
}
