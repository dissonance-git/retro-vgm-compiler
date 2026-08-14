#pragma once

#include <algorithm>
#include <cstdint>

namespace vgmtooling::model {

// Spatial presentation begins with what the source actually preserves. These
// families describe implementation provenance, not a universal synthesis model.
enum class spatial_source_family : std::uint8_t {
    unknown = 0,
    vgm,
    spc,
    psf1,
    gsf,
    usf,
    twosf,
    ncsf,
};

enum class spatial_source_readiness : std::uint8_t {
    unavailable = 0,
    evidence_only,
    isolated_audio_partial,
    isolated_audio_ready,
};

enum class spatial_evidence_authority : std::uint8_t {
    unknown = 0,
    format_authored,
    driver_authored,
    device_authored,
    inferred,
};

struct stereo_route_evidence {
    bool present = false;
    float left_gain = 0.0f;
    float right_gain = 0.0f;
    spatial_evidence_authority authority = spatial_evidence_authority::unknown;
};

// Realtime-safe source evidence carried beside isolated source audio when that
// audio exists. This record intentionally contains no presentation pose.
// Omniphony or another renderer may construct a conservative presentation from
// this evidence, but must not relabel that construction as authored geometry.
struct spatial_source_evidence {
    std::uint64_t source_id = 0;
    std::uint64_t generation = 0;
    spatial_source_family family = spatial_source_family::unknown;

    bool physical_slot_present = false;
    std::uint32_t physical_slot = 0;

    stereo_route_evidence stereo_route{};

    bool effect_send_known = false;
    bool effect_send_enabled = false;

    bool persistent_part_present = false;
    std::uint64_t persistent_part_id = 0;
    float persistent_part_confidence = 0.0f;

    // This may become true only when the source representation itself carries
    // a position. Stereo pan, signed L/R gains, voice number, spectral role or
    // inferred musical identity are not authored 3-D coordinates.
    bool authored_position_present = false;
    float authored_position[3] = {0.0f, 0.0f, 0.0f};
};

struct spatial_source_capabilities {
    spatial_source_readiness readiness = spatial_source_readiness::unavailable;
    bool stable_source_evidence = false;
    bool authored_stereo_route = false;
    bool effect_send_state = false;
    bool isolated_pcm = false;
    bool authored_3d_position = false;
};

// This table is deliberately a statement about the repository's current
// executable evidence, not about what the hardware/platform could expose in
// principle. A reconstructed xSF effective object is not yet a decoded voice.
constexpr spatial_source_capabilities spatial_capabilities_for(
    spatial_source_family family) noexcept {
    switch (family) {
    case spatial_source_family::vgm:
        return {
            spatial_source_readiness::isolated_audio_partial,
            true,  // exact chip/source state exists
            true,  // YM2612 LR and supported PSG routing
            false,
            false, // some PSG/DAC stems exist, but the whole format is not isolated yet
            false,
        };
    case spatial_source_family::spc:
        return {
            spatial_source_readiness::evidence_only,
            true,  // bounded runtime voice episodes exist
            true,  // signed per-voice S-DSP routing is captured
            true,  // echo-send state is captured
            false, // per-voice PCM taps/echo separation are still the render frontier
            false,
        };
    case spatial_source_family::psf1:
    case spatial_source_family::gsf:
    case spatial_source_family::usf:
    case spatial_source_family::twosf:
    case spatial_source_family::ncsf:
        return {
            spatial_source_readiness::unavailable,
            false,
            false,
            false,
            false,
            false,
        };
    case spatial_source_family::unknown:
    default:
        return {};
    }
}

constexpr float clamp_unit_gain(float value) noexcept {
    return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

constexpr bool may_claim_authored_3d(const spatial_source_evidence& source) noexcept {
    return source.authored_position_present;
}

} // namespace vgmtooling::model
