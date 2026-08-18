#pragma once

#include "realtime_musical_omniphony_pipeline.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

enum class realtime_spatial_governor_trace_error {
    none = 0,
    trace_not_valid,
    invalid_sequence,
    invalid_timing,
    invalid_threshold,
    nonfinite_budget,
    renderer_budget_mismatch,
    invalid_source_observation,
    scene_count_mismatch,
    scene_energy_mismatch,
    pair_overlap_mismatch,
    sequence_index_mismatch,
    sequence_sample_rate_mismatch,
    sequence_position_mismatch,
    sequence_budget_mismatch,
};

struct realtime_spatial_governor_trace_validation {
    bool valid = false;
    realtime_spatial_governor_trace_error error =
        realtime_spatial_governor_trace_error::trace_not_valid;
    std::size_t observed_lane_count = 0;
    std::size_t active_lane_count = 0;
    std::size_t active_dry_pair_count = 0;
    float reconstructed_energy_concentration = 0.0f;
    float reconstructed_shared_effect_energy_share = 0.0f;
    float reconstructed_coarse_spectral_overlap = 0.0f;
    float strongest_pair_overlap = 0.0f;
};

struct realtime_spatial_governor_trace_continuity {
    bool valid = false;
    realtime_spatial_governor_trace_error error =
        realtime_spatial_governor_trace_error::trace_not_valid;
};

inline bool governor_trace_near(float left, float right, float tolerance) noexcept
{
    return std::isfinite(left) && std::isfinite(right) &&
        std::fabs(left - right) <= tolerance;
}

inline bool governor_trace_near(double left, double right, double tolerance) noexcept
{
    return std::isfinite(left) && std::isfinite(right) &&
        std::fabs(left - right) <= tolerance;
}

inline bool finite_realtime_spatial_mix_budget(
    const realtime_spatial_mix_budget& budget) noexcept
{
    return std::isfinite(budget.dry_width_scale) &&
        std::isfinite(budget.dry_diffuse_scale) &&
        std::isfinite(budget.depth_scale) &&
        std::isfinite(budget.height_scale) &&
        std::isfinite(budget.shared_wet_strength) &&
        std::isfinite(budget.shared_wet_extent) &&
        std::isfinite(budget.added_externalization_scale);
}

inline bool finite_omniphony_mix_budget(
    const omniphony_source_mix_budget_v1_transport& budget) noexcept
{
    return std::isfinite(budget.depth_scale) &&
        std::isfinite(budget.height_scale) &&
        std::isfinite(budget.shared_wet_strength_scale) &&
        std::isfinite(budget.shared_wet_extent_scale) &&
        std::isfinite(budget.externalization_scale);
}

inline bool same_realtime_spatial_mix_budget(
    const realtime_spatial_mix_budget& left,
    const realtime_spatial_mix_budget& right,
    float tolerance) noexcept
{
    return governor_trace_near(left.dry_width_scale, right.dry_width_scale, tolerance) &&
        governor_trace_near(left.dry_diffuse_scale, right.dry_diffuse_scale, tolerance) &&
        governor_trace_near(left.depth_scale, right.depth_scale, tolerance) &&
        governor_trace_near(left.height_scale, right.height_scale, tolerance) &&
        governor_trace_near(left.shared_wet_strength, right.shared_wet_strength, tolerance) &&
        governor_trace_near(left.shared_wet_extent, right.shared_wet_extent, tolerance) &&
        governor_trace_near(
            left.added_externalization_scale,
            right.added_externalization_scale,
            tolerance);
}

template <std::size_t MaxLanes>
realtime_spatial_governor_trace_validation validate_realtime_spatial_governor_trace(
    const realtime_spatial_governor_trace<MaxLanes>& trace,
    float tolerance = 1.0e-4f) noexcept
{
    realtime_spatial_governor_trace_validation report{};
    if (!trace.valid || trace.lane_count > MaxLanes)
        return report;
    if (trace.sequence_index == 0) {
        report.error = realtime_spatial_governor_trace_error::invalid_sequence;
        return report;
    }

    if (!std::isfinite(trace.sample_rate) || trace.sample_rate <= 0.0) {
        report.error = realtime_spatial_governor_trace_error::invalid_timing;
        return report;
    }
    if (!std::isfinite(trace.active_threshold) || trace.active_threshold < 0.0f ||
        trace.active_threshold > 1.0f) {
        report.error = realtime_spatial_governor_trace_error::invalid_threshold;
        return report;
    }
    if (!std::isfinite(tolerance) || tolerance < 0.0f)
        tolerance = 1.0e-4f;

    if (!finite_realtime_spatial_mix_budget(trace.applied_budget) ||
        !finite_realtime_spatial_mix_budget(trace.learned_budget) ||
        !finite_omniphony_mix_budget(trace.renderer_budget)) {
        report.error = realtime_spatial_governor_trace_error::nonfinite_budget;
        return report;
    }

    const auto expected_renderer_budget = make_omniphony_source_mix_budget(trace.applied_budget);
    if (!governor_trace_near(
            trace.renderer_budget.depth_scale,
            expected_renderer_budget.depth_scale,
            tolerance) ||
        !governor_trace_near(
            trace.renderer_budget.height_scale,
            expected_renderer_budget.height_scale,
            tolerance) ||
        !governor_trace_near(
            trace.renderer_budget.shared_wet_strength_scale,
            expected_renderer_budget.shared_wet_strength_scale,
            tolerance) ||
        !governor_trace_near(
            trace.renderer_budget.shared_wet_extent_scale,
            expected_renderer_budget.shared_wet_extent_scale,
            tolerance) ||
        !governor_trace_near(
            trace.renderer_budget.externalization_scale,
            expected_renderer_budget.externalization_scale,
            tolerance)) {
        report.error = realtime_spatial_governor_trace_error::renderer_budget_mismatch;
        return report;
    }

    double relative_energy_sum = 0.0;
    double concentration = 0.0;
    double shared_wet = 0.0;
    for (std::size_t lane_index = 0; lane_index < trace.lane_count; ++lane_index) {
        const auto& source = trace.sources[lane_index];
        if (!std::isfinite(source.activity) || source.activity < 0.0f || source.activity > 1.0f ||
            !std::isfinite(source.relative_energy) || source.relative_energy < 0.0f ||
            source.relative_energy > 1.0f) {
            report.error = realtime_spatial_governor_trace_error::invalid_source_observation;
            return report;
        }
        for (const float share : source.coarse_band_energy_share) {
            if (!std::isfinite(share) || share < 0.0f || share > 1.0f) {
                report.error = realtime_spatial_governor_trace_error::invalid_source_observation;
                return report;
            }
        }
        if (!source.audio_observed)
            continue;

        ++report.observed_lane_count;
        if (source.activity >= trace.active_threshold)
            ++report.active_lane_count;

        const double relative = static_cast<double>(source.relative_energy);
        relative_energy_sum += relative;
        concentration += relative * relative;
        if (source.lane_kind == spatial_audio_lane_kind::shared_effect_return)
            shared_wet += relative;
    }

    if (report.observed_lane_count != trace.scene.observed_lane_count ||
        report.active_lane_count != trace.scene.active_lane_count) {
        report.error = realtime_spatial_governor_trace_error::scene_count_mismatch;
        return report;
    }

    if (relative_energy_sum > 0.0 &&
        std::fabs(relative_energy_sum - 1.0) > static_cast<double>(tolerance) * 4.0) {
        report.error = realtime_spatial_governor_trace_error::scene_energy_mismatch;
        return report;
    }

    report.reconstructed_energy_concentration = clamp_unit_interval(
        static_cast<float>(concentration));
    report.reconstructed_shared_effect_energy_share = clamp_unit_interval(
        static_cast<float>(shared_wet));
    if (!governor_trace_near(
            report.reconstructed_energy_concentration,
            trace.scene.energy_concentration,
            tolerance * 4.0f) ||
        !governor_trace_near(
            report.reconstructed_shared_effect_energy_share,
            trace.scene.shared_effect_energy_share,
            tolerance * 4.0f)) {
        report.error = realtime_spatial_governor_trace_error::scene_energy_mismatch;
        return report;
    }

    for (std::size_t left = 0; left < trace.lane_count; ++left) {
        for (std::size_t right = left + 1; right < trace.lane_count; ++right) {
            realtime_spatial_overlap_pair_observation pair{};
            if (!trace.pair(left, right, pair))
                continue;
            ++report.active_dry_pair_count;
            report.strongest_pair_overlap = std::max(
                report.strongest_pair_overlap,
                pair.coarse_spectral_overlap);
        }
    }
    report.reconstructed_coarse_spectral_overlap =
        trace.reconstructed_coarse_spectral_overlap();
    if (!governor_trace_near(
            report.reconstructed_coarse_spectral_overlap,
            trace.scene.coarse_spectral_overlap,
            tolerance * 4.0f)) {
        report.error = realtime_spatial_governor_trace_error::pair_overlap_mismatch;
        return report;
    }

    report.valid = true;
    report.error = realtime_spatial_governor_trace_error::none;
    return report;
}

template <std::size_t MaxLanes>
realtime_spatial_governor_trace_continuity validate_realtime_spatial_governor_trace_continuity(
    const realtime_spatial_governor_trace<MaxLanes>& previous,
    const realtime_spatial_governor_trace<MaxLanes>& current,
    float tolerance = 1.0e-4f) noexcept
{
    realtime_spatial_governor_trace_continuity report{};
    const auto previous_validation =
        validate_realtime_spatial_governor_trace(previous, tolerance);
    const auto current_validation =
        validate_realtime_spatial_governor_trace(current, tolerance);
    if (!previous_validation.valid) {
        report.error = previous_validation.error;
        return report;
    }
    if (!current_validation.valid) {
        report.error = current_validation.error;
        return report;
    }

    if (previous.sequence_index == std::numeric_limits<std::uint64_t>::max() ||
        current.sequence_index != previous.sequence_index + 1u) {
        report.error = realtime_spatial_governor_trace_error::sequence_index_mismatch;
        return report;
    }
    if (!governor_trace_near(previous.sample_rate, current.sample_rate, 1.0e-9)) {
        report.error = realtime_spatial_governor_trace_error::sequence_sample_rate_mismatch;
        return report;
    }

    const std::uint64_t previous_frames = static_cast<std::uint64_t>(previous.frame_count);
    if (previous.absolute_sample_position >
            std::numeric_limits<std::uint64_t>::max() - previous_frames ||
        current.absolute_sample_position != previous.absolute_sample_position + previous_frames) {
        report.error = realtime_spatial_governor_trace_error::sequence_position_mismatch;
        return report;
    }
    if (!same_realtime_spatial_mix_budget(
            current.applied_budget,
            previous.learned_budget,
            tolerance)) {
        report.error = realtime_spatial_governor_trace_error::sequence_budget_mismatch;
        return report;
    }

    report.valid = true;
    report.error = realtime_spatial_governor_trace_error::none;
    return report;
}

} // namespace vgmtooling::model
