#include "../../model/spatial_playback_options.h"

#include <cassert>

using namespace vgmtooling::model;

int main() {
    // 1. Reference source quality + reference stereo.
    spatial_playback_options reference{};
    reference.surround = false;
    reference.enhanced = false;
    assert(resolve_spatial_playback(reference) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(reference));
    assert(!uses_enhanced_renderer(reference));
    assert(!uses_externalization(reference));

    // 2. Enhanced source quality + reference stereo. Source realization may
    // improve, but Spatial remains completely off.
    auto enhanced_stereo = reference;
    enhanced_stereo.enhanced = true;
    assert(resolve_spatial_playback(enhanced_stereo) == spatial_playback_path::reference_stereo);
    assert(!uses_source_renderer(enhanced_stereo));
    assert(uses_enhanced_renderer(enhanced_stereo));
    assert(!uses_externalization(enhanced_stereo));

    // 3. Reference source quality + Spatial. Spatial must not imply Enhanced.
    auto reference_spatial = reference;
    reference_spatial.surround = true;
    reference_spatial.depth = spatial_depth_mode::full;
    assert(resolve_spatial_playback(reference_spatial) == spatial_playback_path::source_full_sphere);
    assert(uses_source_renderer(reference_spatial));
    assert(!uses_enhanced_renderer(reference_spatial));
    assert(uses_externalization(reference_spatial));

    // 4. Enhanced source quality + Spatial. Both independent switches may be on.
    auto enhanced_spatial = reference_spatial;
    enhanced_spatial.enhanced = true;
    assert(resolve_spatial_playback(enhanced_spatial) == spatial_playback_path::source_full_sphere);
    assert(uses_source_renderer(enhanced_spatial));
    assert(uses_enhanced_renderer(enhanced_spatial));
    assert(uses_externalization(enhanced_spatial));

    // Native source-aware routing is an independent presentation depth control.
    auto native_spatial = enhanced_spatial;
    native_spatial.depth = spatial_depth_mode::native;
    assert(resolve_spatial_playback(native_spatial) == spatial_playback_path::source_native_routing);
    assert(uses_source_renderer(native_spatial));
    assert(uses_enhanced_renderer(native_spatial));

    // Externalization is presentation-only and is inert while Spatial is off.
    auto no_externalization = enhanced_spatial;
    no_externalization.externalization = false;
    assert(!uses_externalization(no_externalization));
    no_externalization.surround = false;
    no_externalization.externalization = true;
    assert(!uses_externalization(no_externalization));
    assert(uses_enhanced_renderer(no_externalization));

    return 0;
}
