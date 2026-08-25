#pragma once

#include "part_motif_discovery.h"
#include "part_role_window_inference.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

inline bool role_gesture_in_window(
    const part_gesture_observation& observation,
    const time_span& active) noexcept {
    if (!active.end.has_value())
        return false;
    if (!part_role_same_time_basis(observation.onset, active.start) ||
        !part_role_same_time_basis(observation.onset, *active.end)) {
        return false;
    }
    return observation.onset.tick >= active.start.tick &&
        observation.onset.tick < active.end->tick;
}

inline std::vector<part_gesture_observation> gestures_in_role_window(
    const std::vector<part_gesture_observation>& observations,
    const time_span& active) {
    std::vector<part_gesture_observation> result;
    for (const auto& observation : observations) {
        if (role_gesture_in_window(observation, active))
            result.push_back(observation);
    }
    std::sort(result.begin(), result.end(), [](const auto& first, const auto& second) {
        return first.onset.tick < second.onset.tick;
    });
    return result;
}

struct role_window_repetition_summary {
    std::size_t event_count = 0;
    std::size_t motif_event_count = 0;
    double best_identity_confidence = 0.0;
    double best_rhythm_similarity = 0.0;
    double event_coverage = 0.0;
    bool pitch_comparable = false;
    bool performance_shape_comparable = false;
    std::optional<bounded_role_signal> structural_motif_prominence{};
    std::optional<bounded_role_signal> rhythmic_repetition{};
};

inline role_window_repetition_summary summarize_role_window_repetition(
    const std::vector<part_gesture_observation>& observations,
    part_motif_discovery_policy policy = {}) {
    role_window_repetition_summary result;
    result.event_count = observations.size();
    if (observations.size() < 3)
        return result;

    // Role inference is allowed to notice a rhythm-only recurrence, but the
    // motif layer already caps rhythm-only identity at 0.55. That ceiling is
    // intentionally preserved here rather than promoted by role bookkeeping.
    // Stronger structural prominence is emitted only when motif identity was
    // actually compared through pitch or resolved performed-trajectory shape.
    policy.require_pitch_comparison = false;
    policy.min_identity_confidence = std::min(
        policy.min_identity_confidence,
        rhythm_only_motif_identity_ceiling);

    const auto repeats = discover_repeated_part_motifs(observations, policy);
    if (repeats.empty())
        return result;

    const auto& best = repeats.front();
    result.motif_event_count = best.first.event_count;
    result.best_identity_confidence = best.similarity.identity_confidence;
    result.best_rhythm_similarity = best.similarity.rhythm_similarity;
    result.pitch_comparable = best.similarity.pitch_comparable;
    result.performance_shape_comparable = best.similarity.performance_shape_comparable;

    const std::size_t covered_events = std::min(
        observations.size(),
        best.first.event_count + best.second.event_count);
    result.event_coverage = observations.empty()
        ? 0.0
        : static_cast<double>(covered_events) /
            static_cast<double>(observations.size());

    // Motif identity becomes structural role evidence only when something
    // beyond rhythm grounded the recurrence. Value is the motif-layer identity
    // confidence; confidence is coverage of this synchronized part window.
    if (result.pitch_comparable || result.performance_shape_comparable) {
        result.structural_motif_prominence = bounded_role_signal{
            result.best_identity_confidence,
            result.event_coverage,
        };
    }

    // Value describes how rhythmically similar the repeated cells are.
    // Confidence describes how much of the observed part-window the best
    // recurrence actually covers. A perfect two-cell coincidence in a long,
    // otherwise unrelated passage therefore stays weak.
    result.rhythmic_repetition = bounded_role_signal{
        result.best_rhythm_similarity,
        result.event_coverage,
    };
    return result;
}

inline std::optional<std::pair<double, std::string>>
    coherent_role_window_register_center(
        const std::vector<part_gesture_observation>& observations) {
    if (observations.empty())
        return std::nullopt;

    const std::string basis = observations.front().pitch_basis;
    if (basis.empty())
        return std::nullopt;

    double total = 0.0;
    for (const auto& observation : observations) {
        if (!observation.log2_pitch_coordinate.has_value() ||
            !std::isfinite(*observation.log2_pitch_coordinate) ||
            observation.pitch_basis != basis) {
            return std::nullopt;
        }
        total += *observation.log2_pitch_coordinate;
    }
    return std::pair<double, std::string>{
        total / static_cast<double>(observations.size()),
        basis,
    };
}

inline part_role_window_descriptor make_part_role_window_descriptor_from_gestures(
    const std::vector<part_gesture_observation>& observations,
    node_id part_id,
    time_span active,
    const part_motif_discovery_policy& motif_policy = {}) {
    if (part_id == 0)
        throw std::invalid_argument("gesture-derived role descriptor requires a persistent-part id");
    if (!active.end.has_value() ||
        !part_role_same_time_basis(active.start, *active.end) ||
        active.end->tick <= active.start.tick) {
        throw std::invalid_argument("gesture-derived role descriptor requires a positive bounded window");
    }

    const auto window = gestures_in_role_window(observations, active);
    for (const auto& observation : window) {
        if (observation.part_id != part_id)
            throw std::invalid_argument("gesture-derived role descriptor cannot merge different persistent parts");
    }

    part_role_window_descriptor result;
    result.part_id = part_id;
    result.active = std::move(active);
    result.onset_count = window.size();

    if (const auto register_center = coherent_role_window_register_center(window)) {
        result.register_coordinate = register_center->first;
        result.register_basis = register_center->second;
    }

    const auto repetition = summarize_role_window_repetition(window, motif_policy);
    if (repetition.structural_motif_prominence.has_value())
        result.structural_motif_prominence = repetition.structural_motif_prominence;
    if (repetition.rhythmic_repetition.has_value())
        result.rhythmic_repetition = repetition.rhythmic_repetition;

    return result;
}

} // namespace vgmtooling::model