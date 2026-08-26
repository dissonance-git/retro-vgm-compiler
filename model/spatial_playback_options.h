#pragma once

#include <cstdint>

namespace vgmtooling::model {

// User-facing playback policy shared by the VGM and SPC foobar components.
// Surround is the active product switch. enhanced is retained only as a future
// project seam and is deliberately unavailable in the current private builds.
enum class spatial_playback_path : std::uint8_t {
    reference_stereo = 0,
    source_spatial = 1,
};

struct spatial_playback_options {
    // Existing foobar Surround checkbox. Off is protected reference stereo.
    // On emits the current source-native standard 7.1 presentation bed.
    bool surround = false;

    // Reserved future source-quality preference. The current product hard-gates
    // this off even if an older persisted preference says true.
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

constexpr bool enhanced_playback_available = false;

constexpr bool uses_enhanced_renderer(const spatial_playback_options&) noexcept {
    return enhanced_playback_available;
}

} // namespace vgmtooling::model
