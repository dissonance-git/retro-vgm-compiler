#pragma once

#include <cstdint>

namespace vgmtooling::model {

// User-facing playback policy shared by the VGM and SPC foobar components.
// The plugins already expose one Surround checkbox; renderer topology,
// externalization policy and transport revisions stay internal to Omniphony.
enum class spatial_playback_path : std::uint8_t {
    reference_stereo = 0,
    source_spatial = 1,
};

struct spatial_playback_options {
    // Existing foobar Surround checkbox. Off is the protected stereo/reference
    // presentation. On admits causal source lanes to the chip-native spatial
    // path and then Omniphony. It never silently enables source enhancement.
    bool surround = false;

    // Independent source-native quality switch. Off preserves reference
    // synthesis/reconstruction. On may relax only independently validated
    // implementation ceilings while preserving the same musical object.
    // It never silently enables surround presentation.
    bool enhanced = false;
};

constexpr spatial_playback_path resolve_spatial_playback(
    const spatial_playback_options& options) noexcept {
    return options.surround
        ? spatial_playback_path::source_spatial
        : spatial_playback_path::reference_stereo;
}

constexpr bool uses_source_renderer(const spatial_playback_options& options) noexcept {
    return resolve_spatial_playback(options) == spatial_playback_path::source_spatial;
}

constexpr bool uses_enhanced_renderer(const spatial_playback_options& options) noexcept {
    return options.enhanced;
}

} // namespace vgmtooling::model
