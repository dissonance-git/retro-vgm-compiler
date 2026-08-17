#pragma once

#include "spc_sample_restoration.h"
#include "spc_snesapu_source_trajectory.h"
#include "spc_studio_sample_reconstruction.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

// Exact authored playback extent for one decoded game sample. The END block
// contributes samples through end_sample (exclusive). A loop, when present,
// must lie inside that extent.
struct spc_game_sample_playback_span {
    double start_sample = 0.0;
    double end_sample = 0.0; // exclusive
    snesapu_game_loop_span loop{};

    bool valid() const noexcept {
        if (!std::isfinite(start_sample) || !std::isfinite(end_sample)
            || start_sample < 0.0 || end_sample <= start_sample || !loop.valid())
            return false;
        if (!loop.present)
            return true;
        return loop.start_sample >= start_sample
            && std::abs(loop.end_sample - end_sample) <= 1.0e-12;
    }
};

struct spc_upstream_playback_reconstruction_result {
    double sample = 0.0;
    bool valid = false;
};

namespace detail {

inline bool spc_near_integer(double value, std::int64_t& integer) noexcept {
    if (!std::isfinite(value))
        return false;
    const double rounded = std::round(value);
    if (std::abs(value - rounded) > 1.0e-9
        || rounded < static_cast<double>(std::numeric_limits<std::int64_t>::min())
        || rounded > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
        return false;
    integer = static_cast<std::int64_t>(rounded);
    return true;
}

inline std::int64_t spc_positive_mod(std::int64_t value, std::int64_t modulus) noexcept {
    const std::int64_t remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

struct spc_upstream_playback_boundaries {
    std::int64_t start = 0;
    std::int64_t end = 0;
    std::int64_t loop_start = 0;
    std::int64_t loop_end = 0;
    bool loop_present = false;
    bool valid = false;
};

inline bool spc_upstream_pcm_all_finite(
    const spc_upstream_sample_view& upstream) noexcept
{
    if (!upstream.valid())
        return false;
    for (std::size_t frame = 0; frame < upstream.frame_count; ++frame) {
        if (!std::isfinite(upstream.mono_pcm[frame]))
            return false;
    }
    return true;
}

inline spc_upstream_playback_boundaries resolve_spc_upstream_playback_boundaries(
    const spc_sample_restoration_candidate& candidate,
    const spc_game_sample_playback_span& playback) noexcept
{
    spc_upstream_playback_boundaries out;
    if (!candidate.upstream.valid() || !candidate.coordinate_map.valid()
        || !playback.valid())
        return out;

    const auto& map = candidate.coordinate_map;
    const double mapped_start = map.map_position(playback.start_sample);
    const double mapped_end = map.map_position(playback.end_sample);
    if (!spc_near_integer(mapped_start, out.start)
        || !spc_near_integer(mapped_end, out.end)
        || out.start < 0 || out.end <= out.start
        || out.end > static_cast<std::int64_t>(candidate.upstream.frame_count))
        return out;

    // Exact automatic restoration has one authored game-domain topology. A
    // caller cannot silently turn a one-shot coordinate map into a looping
    // playback span, drop a mapped loop, or supply a different game-loop start
    // merely because both descriptions happen to land on usable upstream PCM.
    if (map.loop_present != playback.loop.present)
        return {};

    out.loop_present = playback.loop.present;
    if (out.loop_present) {
        if (std::abs(map.game_loop_start - playback.loop.start_sample) > 1.0e-9)
            return {};

        const double mapped_loop_from_linear = map.map_position(playback.loop.start_sample);
        if (!std::isfinite(mapped_loop_from_linear)
            || std::abs(mapped_loop_from_linear - map.upstream_loop_start) > 1.0e-9)
            return {};

        const double mapped_loop_end = map.upstream_loop_start
            + (playback.loop.end_sample - playback.loop.start_sample)
                * map.upstream_frames_per_game_sample;
        if (!spc_near_integer(map.upstream_loop_start, out.loop_start)
            || !spc_near_integer(mapped_loop_end, out.loop_end)
            || out.loop_start < out.start
            || out.loop_end != out.end
            || out.loop_end <= out.loop_start)
            return {};
    }

    out.valid = true;
    return out;
}

inline std::int64_t map_spc_virtual_source_index(
    std::int64_t index,
    const spc_upstream_playback_boundaries& boundaries,
    std::uint64_t loop_cycle,
    bool& zero) noexcept
{
    zero = false;
    if (!boundaries.loop_present) {
        if (index < boundaries.start || index >= boundaries.end)
            zero = true;
        return index;
    }

    const std::int64_t loop_length = boundaries.loop_end - boundaries.loop_start;
    if (loop_cycle == 0) {
        if (index < boundaries.start) {
            zero = true;
            return index;
        }
        if (index < boundaries.loop_end)
            return index;
        return boundaries.loop_start
            + spc_positive_mod(index - boundaries.loop_end, loop_length);
    }

    return boundaries.loop_start
        + spc_positive_mod(index - boundaries.loop_start, loop_length);
}

// True when the complete symmetric FIR neighborhood maps to one physically
// contiguous run of upstream PCM. Most steady-state samples satisfy this. Only
// key-on/END/loop-seam neighborhoods need the more expensive per-tap virtual
// topology mapping below.
inline bool spc_upstream_window_is_contiguous(
    std::int64_t first_index,
    std::int64_t end_index,
    const spc_upstream_playback_boundaries& boundaries,
    std::uint64_t loop_cycle) noexcept
{
    if (!boundaries.valid || end_index <= first_index)
        return false;

    if (!boundaries.loop_present || loop_cycle == 0)
        return first_index >= boundaries.start && end_index <= boundaries.end;

    return first_index >= boundaries.loop_start && end_index <= boundaries.loop_end;
}

// Realtime primitive for callers that already resolved immutable source-binding
// geometry under the normal admission contract. SourceFiniteVerified=true is
// reserved for callers that also scanned the complete upstream PCM at setup;
// standalone callers retain the defensive per-tap finite checks by default.
template <bool SourceFiniteVerified = false>
inline spc_upstream_playback_reconstruction_result
reconstruct_spc_upstream_candidate_playback_sample_resolved(
    const spc_sample_restoration_candidate& candidate,
    const spc_game_sample_playback_span& playback,
    const snesapu_source_trajectory_projection& trajectory,
    const spc_upstream_playback_boundaries& boundaries) noexcept
{
    spc_upstream_playback_reconstruction_result result;
    if (!candidate.upstream.valid() || !candidate.coordinate_map.valid()
        || !playback.valid() || !trajectory.valid || trajectory.before_key_on
        || !boundaries.valid)
        return result;

    double position = 0.0;
    if (trajectory.loop_cycle == 0) {
        position = candidate.coordinate_map.map_position(
            trajectory.effective_sample_position);
    } else {
        if (!playback.loop.present || !candidate.coordinate_map.loop_present)
            return result;
        position = candidate.coordinate_map.upstream_loop_start
            + (trajectory.canonical_game_sample_position - playback.loop.start_sample)
                * candidate.coordinate_map.upstream_frames_per_game_sample;
    }
    if (!std::isfinite(position))
        return result;

    double base = std::floor(position);
    std::int64_t center = static_cast<std::int64_t>(base);
    const double fraction = position - base;
    std::size_t phase = static_cast<std::size_t>(
        std::floor(fraction * static_cast<double>(spc_studio_phase_count) + 0.5));
    if (phase >= spc_studio_phase_count) {
        phase = 0;
        ++center;
    }

    const auto& table = spc_studio_table();
    const auto& coefficients = table.phase(phase);
    constexpr std::int64_t first_offset =
        -static_cast<std::int64_t>(spc_studio_tap_count / 2 - 1);
    const std::int64_t first_virtual_index = center + first_offset;
    const std::int64_t end_virtual_index = first_virtual_index
        + static_cast<std::int64_t>(spc_studio_tap_count);

    double weighted = 0.0;
    double weight_sum = 0.0;

    if (spc_upstream_window_is_contiguous(
            first_virtual_index,
            end_virtual_index,
            boundaries,
            trajectory.loop_cycle)) {
        // Steady-state fast path. Preserve the exact 64-tap law and weighted
        // summation order, but avoid per-tap topology work and repeated phase
        // normalization. Production may also omit per-tap finite checks after a
        // complete setup-time PCM scan.
        if (first_virtual_index < 0
            || end_virtual_index
                > static_cast<std::int64_t>(candidate.upstream.frame_count))
            return result;
        const float* source = candidate.upstream.mono_pcm
            + static_cast<std::size_t>(first_virtual_index);
        weight_sum = table.phase_weight_sum(phase);
        for (std::size_t tap = 0; tap < spc_studio_tap_count; ++tap) {
            const float sample = source[tap];
            if constexpr (!SourceFiniteVerified) {
                if (!std::isfinite(sample))
                    return result;
            }
            const double coefficient = static_cast<double>(coefficients[tap]);
            weighted += static_cast<double>(sample) * coefficient;
        }
    } else {
        // Boundary path. Preserve exact authored topology: zero before key-on /
        // after one-shot END and wrap only where the live sample loop says to.
        for (std::size_t tap = 0; tap < spc_studio_tap_count; ++tap) {
            const std::int64_t virtual_index = first_virtual_index
                + static_cast<std::int64_t>(tap);
            bool zero = false;
            const std::int64_t source_index = map_spc_virtual_source_index(
                virtual_index, boundaries, trajectory.loop_cycle, zero);
            const double coefficient = static_cast<double>(coefficients[tap]);
            weight_sum += coefficient;
            if (zero)
                continue;
            if (source_index < 0
                || source_index >= static_cast<std::int64_t>(candidate.upstream.frame_count))
                return result;

            const float sample = candidate.upstream.mono_pcm[
                static_cast<std::size_t>(source_index)];
            if constexpr (!SourceFiniteVerified) {
                if (!std::isfinite(sample))
                    return result;
            }
            weighted += static_cast<double>(sample) * coefficient;
        }
    }

    if (!std::isfinite(weighted) || !std::isfinite(weight_sum)
        || std::abs(weight_sum) < 1.0e-12)
        return result;

    const double scaled = weighted / weight_sum
        * candidate.upstream.game_pcm_units_per_source_unit;
    if (!std::isfinite(scaled))
        return result;

    result.sample = scaled;
    result.valid = true;
    return result;
}

} // namespace detail

// Highest-confidence source sampler with authored key-on/END/LOOP topology.
// The long FIR is evaluated over the *playback trajectory*, not blindly over
// adjacent file frames. Before key-on and after a non-looping END the virtual
// source is zero. On the first pass, pre-loop material remains available to the
// left while right-side lookahead across END wraps to the loop start. After the
// first wrap, the FIR neighborhood is periodic inside the loop.
//
// This first exact implementation deliberately admits only boundaries that map
// to integer upstream frames. Fractional source-domain loop boundaries require
// a separately resampled virtual ring and fail closed here.
inline spc_upstream_playback_reconstruction_result
reconstruct_spc_upstream_candidate_playback_sample(
    const spc_sample_restoration_candidate& candidate,
    const spc_game_sample_playback_span& playback,
    const snesapu_source_trajectory_projection& trajectory) noexcept
{
    const auto boundaries = detail::resolve_spc_upstream_playback_boundaries(
        candidate, playback);
    return detail::reconstruct_spc_upstream_candidate_playback_sample_resolved(
        candidate, playback, trajectory, boundaries);
}

inline spc_upstream_playback_reconstruction_result reconstruct_spc_upstream_playback_sample(
    const spc_sample_restoration_candidate& candidate,
    const spc_game_sample_playback_span& playback,
    const snesapu_source_trajectory_projection& trajectory) noexcept
{
    if (!may_use_spc_sample_restoration_automatically(candidate))
        return {};
    return reconstruct_spc_upstream_candidate_playback_sample(
        candidate, playback, trajectory);
}

} // namespace gameaudio::spc
