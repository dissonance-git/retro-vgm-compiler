#pragma once

#include <algorithm>
#include <cstddef>
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

enum class spatial_audio_lane_kind : std::uint8_t {
    dry_source = 0,
    shared_effect_return = 1,
    reference_mix = 2,
};

// The protected reference mix is a control and audible authority, not another
// spatial object lane. It may travel beside object lanes at a host boundary but
// must never be assigned object geometry or folded into object-memory state.
constexpr bool spatial_audio_lane_is_object_renderable(
    spatial_audio_lane_kind kind) noexcept
{
    return kind != spatial_audio_lane_kind::reference_mix;
}

struct stereo_route_evidence {
    bool present = false;
    float left_gain = 0.0f;
    float right_gain = 0.0f;
    spatial_evidence_authority authority = spatial_evidence_authority::unknown;

    // Arithmetic provenance, not geometry. When true, mono_pcm already carries
    // the native sample-accurate route-gain trajectory. Consumers may still use
    // the signed gains as native pose/polarity evidence, but must not multiply
    // them into the PCM again. This mirrors the downstream Omniphony transport
    // contract while keeping the compiler independent of that renderer.
    bool gain_preapplied = false;
};

// Higher musical/perceptual evidence that may inform a renderer's presentation
// policy. None of these fields are source-authored coordinates. A register or
// role estimate can justify a conservative presentation tendency while keeping
// the resulting 3-D pose explicitly renderer-inferred.
struct spatial_presentation_evidence {
    float foundation = 0.0f;
    float foreground = 0.0f;
    float diffuse = 0.0f;
    float width = 0.0f;
    float vertical_affinity = 0.0f; // [-1, +1], negative=lower, positive=upper
    float confidence = 0.0f;
    spatial_evidence_authority authority = spatial_evidence_authority::inferred;
};

// Realtime-safe source evidence carried beside isolated source audio when that
// audio exists. This record intentionally contains no inferred presentation
// pose. Omniphony or another renderer may construct a presentation from this
// evidence, but must not relabel that construction as authored geometry.
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

    spatial_presentation_evidence presentation{};

    // This may become true only when the source representation itself carries
    // a position. Stereo pan, signed L/R gains, voice number, spectral role or
    // inferred musical identity are not authored 3-D coordinates.
    bool authored_position_present = false;
    float authored_position[3] = {0.0f, 0.0f, 0.0f};
};

// Non-owning realtime view over one mono source lane. Ownership stays with the
// source-specific renderer so this shared model does not impose allocation,
// interleaving, or synthesis semantics on VGM/SPC.
struct spatial_audio_lane_view {
    spatial_audio_lane_kind kind = spatial_audio_lane_kind::dry_source;
    const float* mono_pcm = nullptr;
    spatial_source_evidence evidence{}; // state at block frame 0

    // Optional per-frame evidence-validity mask for mono_pcm. A null pointer
    // means the source owner asserts that every PCM frame in this block is
    // available. A zero entry means the sample value must not be interpreted as
    // observed source audio, even if the transport buffer contains zero there.
    const std::uint8_t* availability = nullptr;
};

// A source's authored/device route may change inside one audio block. This
// record changes the evidence for one lane beginning at frame_offset. The event
// is evidence-time state, not a renderer pose and not a new source identity.
struct spatial_source_evidence_event {
    std::size_t frame_offset = 0;
    std::size_t lane_index = 0;
    spatial_source_evidence evidence{};
};

struct spatial_source_block_view {
    const spatial_audio_lane_view* lanes = nullptr;
    std::size_t lane_count = 0;
    std::size_t frame_count = 0;

    // Optional ordered evidence automation. Each event applies from its frame
    // boundary onward. Source-specific adapters must preserve event ordering and
    // fail closed rather than silently sorting ambiguous timelines.
    const spatial_source_evidence_event* evidence_events = nullptr;
    std::size_t evidence_event_count = 0;
};

// Audio separability is deliberately more precise than a single "has stems"
// bit. A causal source trajectory can be known even when it cannot be rendered
// as an independently additive stem. Shared feedback, cross-resource
// modulation and finite-width/nonlinear mixing are all counterexamples.
struct spatial_source_capabilities {
    spatial_source_readiness readiness = spatial_source_readiness::unavailable;
    bool stable_source_evidence = false;
    bool authored_stereo_route = false;
    bool effect_send_state = false;

    // At least one source path in this family currently exposes isolated dry
    // PCM. For a partially ready family this does not imply every source does.
    bool isolated_dry_pcm = false;

    // A separately observable shared wet/effect return is available. This is
    // intentionally distinct from knowing only the per-source send state.
    bool shared_effect_return = false;

    // Summing the isolated source lanes reproduces the protected reference mix
    // under the declared arithmetic boundary. Leave false whenever coupling,
    // shared feedback, saturation/nonlinear mixing or missing lanes prevent it.
    bool exact_linear_recomposition = false;

    // A protected reference renderer/mix exists and should remain available as
    // the scientific control while source-aware presentation is developed.
    bool protected_reference_mix = false;

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
            true,  // selected PSG/DAC source lanes exist
            false, // no family-wide shared wet-return contract
            false, // VGM spans devices where source contributions need not add linearly
            true,  // imported libvgm playback remains the protected reference
            false,
        };
    case spatial_source_family::spc:
        return {
            spatial_source_readiness::isolated_audio_partial,
            true,  // bounded runtime voice episodes exist
            true,  // signed per-voice S-DSP routing is captured
            true,  // echo-send state is captured
            true,  // exact pre-pan dry PCM exists on the native/SNESAPU paths
            true,  // SNESAPU SRCE v2 exposes the final shared post-EVOL wet return
            false, // PMON/shared echo/finite arithmetic forbid a blanket sum-of-stems claim
            true,  // protected reference playback remains the control
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

constexpr float clamp_unit_interval(float value) noexcept {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

constexpr bool may_claim_authored_3d(const spatial_source_evidence& source) noexcept {
    return source.authored_position_present;
}

} // namespace vgmtooling::model
