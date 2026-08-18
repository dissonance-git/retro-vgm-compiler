#include "../../model/spatial_playback_options.h"

#include <cassert>

using namespace vgmtooling::model;

int main() {
    // Reference source quality + reference stereo.
    spatial_playback_options reference{};
    assert(resolve_spatial_playback(reference) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(reference));
    assert(!uses_enhanced_renderer(reference));

    // Source enhancement is independent of presentation.
    auto enhanced_stereo = reference;
    enhanced_stereo.enhanced = true;
    assert(resolve_spatial_playback(enhanced_stereo) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(enhanced_stereo));
    assert(uses_enhanced_renderer(enhanced_stereo));

    // The existing foobar Surround option is the entire user-facing spatial
    // decision. It admits the source-native path but never implies enhancement.
    auto reference_surround = reference;
    reference_surround.surround = true;
    assert(resolve_spatial_playback(reference_surround) == spatial_playback_path::source_spatial);
    assert(uses_source_renderer(reference_surround));
    assert(!uses_enhanced_renderer(reference_surround));

    // The two switches remain orthogonal.
    auto enhanced_surround = reference_surround;
    enhanced_surround.enhanced = true;
    assert(resolve_spatial_playback(enhanced_surround) == spatial_playback_path::source_spatial);
    assert(uses_source_renderer(enhanced_surround));
    assert(uses_enhanced_renderer(enhanced_surround));

    enhanced_surround.surround = false;
    assert(resolve_spatial_playback(enhanced_surround) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(enhanced_surround));
    assert(uses_enhanced_renderer(enhanced_surround));

    return 0;
}
