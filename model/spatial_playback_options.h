#pragma once

#include <cstdint>

namespace vgmtooling::model {

// User-facing depth choice shared by the VGM and SPC foobar components.
// This is presentation policy only. It does not change source decoding,
// synthesis, authored stereo routing, or historical shared-DSP state.
enum class spatial_depth_mode : std::uint8_t {
    native = 0,
    full = 1,
};

// Resolved playback path. Keeping the reference path explicit prevents a
// checked/unchecked UI control from becoming an undocumented DSP blend.
enum class spatial_playback_path : std::uint8_t {
    reference_stereo = 0,
    source_native_routing = 1,
    source_full_sphere = 2,
};

struct spatial_playback_options {
    // Existing foobar "Surround" checkbox. Off is the protected historical
    // stereo/reference path. On admits causal source lanes to Omniphony.
    bool surround = false;

    // Independent externalization cue. This enables Omniphony's conservative
    // early-reflection field, never the source format's own echo/reverb.
    bool externalization = true;

    // Full is the intended listening default once Surround is enabled. Native
    // remains available as the useful source-aware control condition.
    spatial_depth_mode depth = spatial_depth_mode::full;
};

constexpr spatial_playback_path resolve_spatial_playback(
    const spatial_playback_options& options) noexcept {
    if (!options.surround)
        return spatial_playback_path::reference_stereo;

    return options.depth == spatial_depth_mode::native
        ? spatial_playback_path::source_native_routing
        : spatial_playback_path::source_full_sphere;
}

constexpr bool uses_source_renderer(const spatial_playback_options& options) noexcept {
    return resolve_spatial_playback(options) != spatial_playback_path::reference_stereo;
}

constexpr bool uses_externalization(const spatial_playback_options& options) noexcept {
    // Externalization has no effect while the protected reference path is in
    // use. This also avoids surprising room cues if the user merely toggles
    // the saved Externalization preference while Surround is off.
    return uses_source_renderer(options) && options.externalization;
}

} // namespace vgmtooling::model
