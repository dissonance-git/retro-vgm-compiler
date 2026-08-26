#include "../../model/spatial_playback_options.h"

#include <cassert>

using namespace vgmtooling::model;

int main() {
    spatial_playback_options reference{};
    assert(resolve_spatial_playback(reference) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(reference));
    assert(!enhanced_playback_available);
    assert(!uses_enhanced_renderer(reference));

    // An old persisted enhanced=true value cannot reactivate the later project.
    auto stale_enhanced = reference;
    stale_enhanced.enhanced = true;
    assert(resolve_spatial_playback(stale_enhanced) == spatial_playback_path::reference_stereo);
    assert(!uses_enhanced_renderer(stale_enhanced));

    auto surround = reference;
    surround.surround = true;
    assert(resolve_spatial_playback(surround) == spatial_playback_path::source_spatial);
    assert(uses_source_renderer(surround));
    assert(!uses_enhanced_renderer(surround));

    surround.enhanced = true;
    assert(resolve_spatial_playback(surround) == spatial_playback_path::source_spatial);
    assert(uses_source_renderer(surround));
    assert(!uses_enhanced_renderer(surround));
    return 0;
}
