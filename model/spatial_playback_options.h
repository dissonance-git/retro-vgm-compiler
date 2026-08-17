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

// Resolved spatial playback path. Source-native enhancement is deliberately
// orthogonal to this choice and may be enabled with any spatial path.
enum class spatial_playback_path : std::uint8_t {
    reference_stereo = 0,
    source_native_routing = 1,
    source_full_sphere = 2,
};

struct spatial_playback_options {
    // Existing foobar "Surround" checkbox. Off is the protected historical
    // stereo/reference presentation. On admits causal source lanes to
    // Omniphony. It must never silently enable source enhancement.
    bool surround = false;

    // Independent source-native quality switch. Off preserves reference
    // synthesis/reconstruction. On may relax only independently validated
    // implementation ceilings while preserving the same musical object.
    // It must never silently enable spatial presentation.
    bool enhanced = false;

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

constexpr bool uses_enhanced_renderer(const spatial_playback_options& options) noexcept {
    return options.enhanced;
}

constexpr bool uses_externalization(const spatial_playback_options& options) noexcept {
    // Externalization has no effect while the protected reference spatial path
    // is in use. Enhancement is unrelated and does not change this condition.
    return uses_source_renderer(options) && options.externalization;
}

} // namespace vgmtooling::model
