#pragma once

#include "part_motif_profile.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class cross_realization_correspondence_kind : std::uint8_t {
    weak_relation = 0,
    synchronous_unison_doubling_candidate,
    synchronous_octave_doubling_candidate,
    delayed_shadow_candidate,
    transformed_doubling_candidate,
    independent_line_candidate,
};

struct cross_realization_part_correspondence_hypothesis {
    node_id first_part_id = 0;
    node_id second_part_id = 0;
    std::string first_realization_family;
    std::string second_realization_family;
    cross_realization_correspondence_kind kind =
        cross_realization_correspondence_kind::weak_relation;
    part_motif_similarity motif_similarity{};
    bool absolute_pitch_offset_comparable = false;
    double median_pitch_offset_octaves = 0.0;
    double pitch_offset_dispersion_octaves = 0.0;
    double median_onset_lag_ticks = 0.0;
    double normalized_onset_lag = 0.0;
    double normalized_lag_dispersion = 0.0;
    bool timing_correspondence_grounded = false;
    bool pitch_correspondence_grounded = false;
    double confidence = 0.0;
};

constexpr double cross_realization_motif_threshold = 0.78;
constexpr double cross_realization_strong_motif_threshold = 0.85;
constexpr double cross_realization_synchronous_lag_ratio = 0.15;
constexpr double cross_realization_max_shadow_lag_ratio = 1.50;
constexpr double cross_realization_lag_dispersion_threshold = 0.12;
constexpr double cross_realization_pitch_tolerance_octaves = 35.0 / 1200.0;
constexpr double cross_realization_structural_only_ceiling = 0.74;

inline const char* to_string(cross_realization_correspondence_kind kind) noexcept {
    switch (kind) {
    case cross_realization_correspondence_kind::weak_relation:
        return "weak_relation";
    case cross_realization_correspondence_kind::synchronous_unison_doubling_candidate:
        return "synchronous_unison_doubling_candidate";
    case cross_realization_correspondence_kind::synchronous_octave_doubling_candidate:
        return "synchronous_octave_doubling_candidate";
    case cross_realization_correspondence_kind::delayed_shadow_candidate:
        return "delayed_shadow_candidate";
    case cross_realization_correspondence_kind::transformed_doubling_candidate:
        return "transformed_doubling_candidate";
    case cross_realization_correspondence_kind::independent_line_candidate:
        return "independent_line_candidate";
    }
    return "unknown";
}

inline double median_any(std::vector<double> values) {
    if (values.empty())
        throw std::invalid_argument("correspondence median requires values");
    for (double value : values) {
        if (!std::isfinite(value))
            throw std::invalid_argument("correspondence values must be finite");
    }
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if ((values.size() & 1u) != 0u)
        return values[middle];
    return 0.5 * (values[middle - 1] + values[middle]);
}

inline double median_absolute_deviation(
    const std::vector<double>& values,
    double center) {
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (double value : values)
        deviations.push_back(std::fabs(value - center));
    return median_any(std::move(deviations));
}

inline bool same_cross_realization_time_basis(
    const part_gesture_observation& first,
    const part_gesture_observation& second) noexcept {
    return first.onset.domain == second.onset.domain &&
        first.onset.tick_rate == second.onset.tick_rate &&
        first.onset.loop_iteration == second.onset.loop_iteration;
}

inline cross_realization_part_correspondence_hypothesis
infer_cross_realization_part_correspondence(
    const std::vector<part_gesture_observation>& first,
    const std::vector<part_gesture_observation>& second,
    std::string first_realization_family,
    std::string second_realization_family) {
    if (first.size() < 3 || first.size() != second.size())
        throw std::invalid_argument(
            "cross-realization correspondence requires equally sized gesture windows of at least three events");
    if (first_realization_family.empty() || second_realization_family.empty())
        throw std::invalid_argument(
            "cross-realization correspondence requires realization-family identities");
    if (first_realization_family == second_realization_family)
        throw std::invalid_argument(
            "cross-realization correspondence requires distinct realization families");

    const node_id first_part = first.front().part_id;
    const node_id second_part = second.front().part_id;
    if (first_part == 0 || second_part == 0 || first_part == second_part)
        throw std::invalid_argument(
            "cross-realization correspondence requires two distinct persistent parts");

    for (std::size_t index = 0; index < first.size(); ++index) {
        if (first[index].part_id != first_part || second[index].part_id != second_part)
            throw std::invalid_argument(
                "correspondence windows cannot silently merge persistent parts");
        if (!same_cross_realization_time_basis(first[index], second[index]) ||
            !same_cross_realization_time_basis(first.front(), first[index])) {
            throw std::invalid_argument(
                "cross-realization correspondence requires one compatible local time basis");
        }
    }

    const auto first_profile = make_part_motif_profile(first);
    const auto second_profile = make_part_motif_profile(second);
    const auto similarity = compare_part_motif_profiles(first_profile, second_profile);

    std::vector<double> first_iois;
    std::vector<double> lags;
    first_iois.reserve(first.size() - 1);
    lags.reserve(first.size());
    for (std::size_t index = 0; index < first.size(); ++index) {
        lags.push_back(static_cast<double>(
            second[index].onset.tick - first[index].onset.tick));
        if (index != 0) {
            const auto delta = first[index].onset.tick - first[index - 1].onset.tick;
            if (delta <= 0)
                throw std::invalid_argument(
                    "initiating correspondence window requires increasing onsets");
            first_iois.push_back(static_cast<double>(delta));
        }
    }
    const double reference_ioi = median_positive(first_iois);
    const double median_lag = median_any(lags);
    const double lag_dispersion = median_absolute_deviation(lags, median_lag);

    cross_realization_part_correspondence_hypothesis result;
    result.first_part_id = first_part;
    result.second_part_id = second_part;
    result.first_realization_family = std::move(first_realization_family);
    result.second_realization_family = std::move(second_realization_family);
    result.motif_similarity = similarity;
    result.median_onset_lag_ticks = median_lag;
    result.normalized_onset_lag = median_lag / reference_ioi;
    result.normalized_lag_dispersion = lag_dispersion / reference_ioi;
    result.timing_correspondence_grounded =
        result.normalized_lag_dispersion <= cross_realization_lag_dispersion_threshold;

    std::vector<double> pitch_offsets;
    bool absolute_pitch_comparable = true;
    pitch_offsets.reserve(first.size());
    for (std::size_t index = 0; index < first.size(); ++index) {
        const auto& a = first[index];
        const auto& b = second[index];
        if (!a.log2_pitch_coordinate.has_value() ||
            !b.log2_pitch_coordinate.has_value() ||
            a.pitch_basis != "absolute_performed_frequency_hz" ||
            b.pitch_basis != "absolute_performed_frequency_hz") {
            absolute_pitch_comparable = false;
            break;
        }
        pitch_offsets.push_back(
            *b.log2_pitch_coordinate - *a.log2_pitch_coordinate);
    }

    if (absolute_pitch_comparable) {
        result.absolute_pitch_offset_comparable = true;
        result.median_pitch_offset_octaves = median_any(pitch_offsets);
        result.pitch_offset_dispersion_octaves = median_absolute_deviation(
            pitch_offsets,
            result.median_pitch_offset_octaves);
        result.pitch_correspondence_grounded =
            result.pitch_offset_dispersion_octaves <=
            cross_realization_pitch_tolerance_octaves;
    }

    const double abs_lag = std::fabs(result.normalized_onset_lag);
    const bool synchronous =
        result.timing_correspondence_grounded &&
        abs_lag <= cross_realization_synchronous_lag_ratio;
    const bool delayed_but_corresponding =
        result.timing_correspondence_grounded &&
        abs_lag > cross_realization_synchronous_lag_ratio &&
        abs_lag <= cross_realization_max_shadow_lag_ratio;

    bool near_unison = false;
    bool near_octave = false;
    if (result.pitch_correspondence_grounded) {
        const double offset = result.median_pitch_offset_octaves;
        near_unison = std::fabs(offset) <= cross_realization_pitch_tolerance_octaves;
        const double nearest_octave = std::round(offset);
        near_octave = std::fabs(nearest_octave) >= 1.0 &&
            std::fabs(offset - nearest_octave) <=
                cross_realization_pitch_tolerance_octaves;
    }

    if (similarity.identity_confidence >= cross_realization_strong_motif_threshold &&
        synchronous && near_unison) {
        result.kind =
            cross_realization_correspondence_kind::synchronous_unison_doubling_candidate;
    } else if (similarity.identity_confidence >= cross_realization_strong_motif_threshold &&
               synchronous && near_octave) {
        result.kind =
            cross_realization_correspondence_kind::synchronous_octave_doubling_candidate;
    } else if (similarity.identity_confidence >= cross_realization_strong_motif_threshold &&
               delayed_but_corresponding && (near_unison || near_octave)) {
        result.kind =
            cross_realization_correspondence_kind::delayed_shadow_candidate;
    } else if (similarity.identity_confidence >= cross_realization_motif_threshold &&
               result.timing_correspondence_grounded) {
        result.kind =
            cross_realization_correspondence_kind::transformed_doubling_candidate;
    } else if (similarity.combined_similarity < 0.60) {
        result.kind =
            cross_realization_correspondence_kind::independent_line_candidate;
    } else {
        result.kind = cross_realization_correspondence_kind::weak_relation;
    }

    // This first pass uses structural motif/timing/pitch correspondence only.
    // Auditory fusion, authored source correspondence, or explicit orchestration
    // evidence may strengthen a future layer, but structural geometry alone may
    // not exceed the single-domain ceiling.
    result.confidence = std::min(
        similarity.identity_confidence,
        cross_realization_structural_only_ceiling);
    if (!result.timing_correspondence_grounded &&
        result.kind != cross_realization_correspondence_kind::independent_line_candidate) {
        result.confidence = std::min(result.confidence, 0.49);
    }
    return result;
}

} // namespace vgmtooling::model
