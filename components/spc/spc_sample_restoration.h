#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Mirrors the durable A..F ladder in docs/source-native-enhanced-rendering.md.
// Ordering is intentional: lower numeric values carry stronger historical
// support. This is evidence for a candidate realization, never a claim that a
// pristine source file is automatically the intended final SNES timbre.
enum class spc_sample_restoration_evidence : std::uint8_t {
    documented_creator_intent = 0,     // A
    same_production_source = 1,        // B
    exact_upstream_source = 2,         // C
    deterministic_ceiling = 3,        // D
    cross_source_inference = 4,        // E
    aesthetic_hypothesis = 5,          // F
    unknown = 6,
};

enum class spc_sample_lineage_relation : std::uint8_t {
    unknown = 0,
    exact_pre_brr_source,
    exact_source_after_game_preparation,
    same_preset_or_library_variant,
    inferred_relative,
};

enum class spc_sample_restoration_permission : std::uint8_t {
    reference_only = 0,
    reversible_experiment,
    source_supported_automatic,
};

// Content identities are supplied by the corpus/shell. The core deliberately
// does not choose a hash algorithm or infer identity from file names.
struct spc_sample_content_identity {
    std::uint64_t high = 0;
    std::uint64_t low = 0;

    constexpr bool present() const noexcept { return high != 0 || low != 0; }
};

constexpr bool same_spc_sample_content_identity(
    const spc_sample_content_identity& left,
    const spc_sample_content_identity& right) noexcept {
    return left.high == right.high && left.low == right.low;
}

struct spc_upstream_sample_view {
    const float* mono_pcm = nullptr;
    std::size_t frame_count = 0;
    double sample_rate_hz = 0.0;

    bool valid() const noexcept {
        return mono_pcm != nullptr
            && frame_count != 0
            && std::isfinite(sample_rate_hz)
            && sample_rate_hz > 0.0;
    }
};

// Exact coordinate map from the historical decoded-game-sample axis into an
// upstream source. This represents known preparation such as trimming and
// resampling without altering the game's pitch/event trajectory.
struct spc_sample_coordinate_map {
    // upstream_frame = upstream_origin +
    //                  (game_sample_position - game_origin) * scale
    double game_origin = 0.0;
    double upstream_origin = 0.0;
    double upstream_frames_per_game_sample = 0.0;

    bool loop_present = false;
    double game_loop_start = 0.0;
    double upstream_loop_start = 0.0;

    // True only when every identity-bearing preparation between upstream source
    // and game instrument that the enhanced path intends to retain is either
    // represented here or replayed elsewhere by an exact transform.
    bool preparation_chain_exact = false;

    bool valid() const noexcept {
        if (!std::isfinite(game_origin) || !std::isfinite(upstream_origin) ||
            !std::isfinite(upstream_frames_per_game_sample) ||
            upstream_frames_per_game_sample <= 0.0)
            return false;
        if (loop_present &&
            (!std::isfinite(game_loop_start) || !std::isfinite(upstream_loop_start)))
            return false;
        return true;
    }

    double map_position(double game_sample_position) const noexcept {
        if (!valid() || !std::isfinite(game_sample_position))
            return 0.0;
        return upstream_origin
            + (game_sample_position - game_origin) * upstream_frames_per_game_sample;
    }

    double map_loop_start() const noexcept {
        if (!valid() || !loop_present)
            return 0.0;
        return upstream_loop_start;
    }
};

struct spc_sample_restoration_candidate {
    spc_sample_content_identity game_brr_identity{};
    spc_sample_content_identity upstream_identity{};
    spc_sample_lineage_relation relation = spc_sample_lineage_relation::unknown;
    spc_sample_restoration_evidence evidence = spc_sample_restoration_evidence::unknown;
    spc_upstream_sample_view upstream{};
    spc_sample_coordinate_map coordinate_map{};

    // Independent validation that the candidate still behaves as the same
    // instrument after the documented preparation chain is preserved. This is
    // not a historical-intent claim; it is the project's own bounded identity
    // test result.
    bool identity_validation_passed = false;
};

constexpr bool spc_has_exact_upstream_lineage(
    spc_sample_lineage_relation relation) noexcept {
    return relation == spc_sample_lineage_relation::exact_pre_brr_source
        || relation == spc_sample_lineage_relation::exact_source_after_game_preparation;
}

inline spc_sample_restoration_permission classify_spc_sample_restoration(
    const spc_sample_restoration_candidate& candidate) noexcept {
    if (!candidate.game_brr_identity.present() ||
        !candidate.upstream_identity.present() ||
        !candidate.upstream.valid() ||
        !candidate.coordinate_map.valid() ||
        !candidate.coordinate_map.preparation_chain_exact ||
        !spc_has_exact_upstream_lineage(candidate.relation))
        return spc_sample_restoration_permission::reference_only;

    // C or stronger can enter normal Enhanced playback once the same-instrument
    // validation has also passed. D..F remain explicit experiments even if a
    // usable source waveform and deterministic map happen to exist.
    const bool source_supported =
        static_cast<std::uint8_t>(candidate.evidence)
            <= static_cast<std::uint8_t>(spc_sample_restoration_evidence::exact_upstream_source);
    if (source_supported && candidate.identity_validation_passed)
        return spc_sample_restoration_permission::source_supported_automatic;

    return spc_sample_restoration_permission::reversible_experiment;
}

inline bool may_use_spc_sample_restoration_automatically(
    const spc_sample_restoration_candidate& candidate) noexcept {
    return classify_spc_sample_restoration(candidate)
        == spc_sample_restoration_permission::source_supported_automatic;
}

// Validate one mapped coordinate before an enhanced sampler dereferences it.
// Fractional positions are allowed because the high-quality reconstruction
// stage owns interpolation in the upstream source domain.
inline bool spc_upstream_position_available(
    const spc_sample_restoration_candidate& candidate,
    double game_sample_position) noexcept {
    if (!candidate.upstream.valid() || !candidate.coordinate_map.valid() ||
        !std::isfinite(game_sample_position))
        return false;
    const double mapped = candidate.coordinate_map.map_position(game_sample_position);
    return std::isfinite(mapped)
        && mapped >= 0.0
        && mapped <= static_cast<double>(candidate.upstream.frame_count - 1u);
}

} // namespace gameaudio::spc
