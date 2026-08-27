#pragma once

#include <cstdint>

namespace vgmtooling::model {

// User-facing playback policy shared by the VGM and SPC foobar components.
// Surround is the only current product switch at this layer.
enum class spatial_playback_path : std::uint8_t {
    reference_stereo = 0,
    source_spatial = 1,
};

struct spatial_playback_options {
    // Existing foobar Surround checkbox. Off is protected reference stereo.
    // On emits the current source-native spatial presentation path.
    bool surround = false;
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

} // namespace vgmtooling::model
