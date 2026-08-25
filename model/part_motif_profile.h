#pragma once

#include "musical_execution_graph.h"
#include "pitch_motion_articulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace vgmtooling::model {

struct part_gesture_performance_shape {
    pitch_motion_articulation_kind kind = pitch_motion_articulation_kind::steady_pitch;
    double pitch_range_semitones = 0.0;
    double net_motion_semitones = 0.0;
    std::size_t direction_changes = 0;
    double confidence = 1.0;
};

inline bool is_resolved_part_gesture_performance_shape(
    const part_gesture_performance_shape& shape) noexcept {
    return shape.kind != pitch_motion_articulation_kind::in_episode_pitch_change_unresolved &&
           shape.kind != pitch_motion_articulation_kind::rearticulation_boundary;
}

struct part_gesture_observation {
    node_id source_node = 0;
    node_id part_id = 0;
    time_coordinate onset{};
    std::optional<double> log2_pitch_coordinate{};
    std::string pitch_basis;
    std::string interval_semantics;
    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
    std::optional<part_gesture_performance_shape> performance_shape{};
};

struct part_motif_profile {
    node_id part_id = 0;
    std::vector<node_id> source_nodes;
    std::vector<double> normalized_inter_onset_intervals;
    std::optional<std::vector<double>> interval_octaves{};
    std::optional<std::vector<std::int8_t>> pitch_contour{};
    std::string pitch_basis;
    std::string interval_semantics;
    std::optional<double> pitch_range_octaves{};
    std::optional<std::vector<part_gesture_performance_shape>> performance_shapes{};
    evidence_status status = evidence_status::derived;
    double evidence_confidence = 1.0;
};

struct part_motif_similarity {
    std::optional<double> interval_similarity{};
    double rhythm_similarity = 0.0;
    std::optional<double> contour_similarity{};
    std::optional<double> performance_shape_similarity{};
    double combined_similarity = 0.0;
    double identity_confidence = 0.0;
    bool pitch_comparable = false;
    bool performance_shape_comparable = false;
    bool transposition_invariant = true;
    bool tempo_scale_invariant = true;
    double evidence_confidence = 1.0;
};

constexpr double rhythm_only_motif_identity_ceiling = 0.55;

// A motif cannot be stronger evidence than the persistent musical-part identity
// that licensed grouping its physical episodes in the first place. Keep this
// reader in the shared motif model so every source-family adapter inherits the
// same epistemic bound.
struct persistent_part_motif_evidence_bound {
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
};

inline persistent_part_motif_evidence_bound read_persistent_part_motif_evidence(
    const node& part) {
    if (part.kind != node_kind::part)
        throw std::invalid_argument("part motif evidence requires a persistent-part node");

    const attribute* scope_item = nullptr;
    for (const auto& item : part.attributes) {
        if (item.key == "identity_scope") {
            scope_item = &item;
            break;
        }
    }
    if (scope_item == nullptr)
        throw std::invalid_argument("persistent part is missing its identity scope evidence");
    const auto* scope = std::get_if<std::string>(&scope_item->value);
    if (scope == nullptr || *scope != "persistent_musical_part")
        throw std::invalid_argument("part motif requires persistent musical-part identity evidence");
    if (!std::isfinite(scope_item->confidence) ||
        scope_item->confidence < 0.0 || scope_item->confidence > 1.0) {
        throw std::invalid_argument("persistent-part identity confidence must be finite and in [0, 1]");
    }
    return {scope_item->status, scope_item->confidence};
}

inline evidence_status weaker_part_motif_evidence_status(
    evidence_status first,
    evidence_status second) noexcept {
    return static_cast<evidence_status>(std::max(
        static_cast<std::uint8_t>(first),
        static_cast<std::uint8_t>(second)));
}

inline double median_positive(std::vector<double> values) {
    values.erase(
        std::remove_if(values.begin(), values.end(), [](double value) {
            return !std::isfinite(value) || value <= 0.0;
        }),
        values.end());
    if (values.empty())
        throw std::invalid_argument("motif timing requires at least one positive interval");
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if ((values.size() & 1u) != 0u)
        return values[middle];
    return 0.5 * (values[middle - 1] + values[middle]);
}

inline part_motif_profile make_part_motif_profile(
    const std::vector<part_gesture_observation>& observations) {
    if (observations.size() < 3)
        throw std::invalid_argument("part motif profile requires at least three observations");

    const node_id part_id = observations.front().part_id;
    if (part_id == 0)
        throw std::invalid_argument("part motif observations require a persistent-part id");

    const time_domain time_domain_value = observations.front().onset.domain;
    const std::uint64_t tick_rate = observations.front().onset.tick_rate;
    const std::int64_t loop_iteration = observations.front().onset.loop_iteration;

    std::vector<double> iois;
    iois.reserve(observations.size() - 1);
    evidence_status weakest_status = observations.front().status;
    double evidence_confidence = 1.0;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const auto& observation = observations[index];
        if (observation.source_node == 0)
            throw std::invalid_argument("part motif observations require source nodes");
        if (observation.part_id != part_id)
            throw std::invalid_argument("one motif profile cannot silently merge different persistent parts");
        if (observation.onset.domain != time_domain_value ||
            observation.onset.tick_rate != tick_rate ||
            observation.onset.loop_iteration != loop_iteration) {
            throw std::invalid_argument("motif observations require one compatible local time and loop basis");
        }
        if (observation.confidence < 0.0 || observation.confidence > 1.0)
            throw std::invalid_argument("motif observation confidence must be in [0, 1]");
        if (observation.performance_shape.has_value()) {
            const auto& shape = *observation.performance_shape;
            if (!is_resolved_part_gesture_performance_shape(shape))
                throw std::invalid_argument("motif observation performance shape must be resolved");
            if (!std::isfinite(shape.pitch_range_semitones) || shape.pitch_range_semitones < 0.0 ||
                !std::isfinite(shape.net_motion_semitones) ||
                !std::isfinite(shape.confidence) || shape.confidence < 0.0 || shape.confidence > 1.0) {
                throw std::invalid_argument("motif observation performance-shape confidence and motion must be finite");
            }
            evidence_confidence = std::min(evidence_confidence, shape.confidence);
        }
        weakest_status = static_cast<evidence_status>(std::max(
            static_cast<std::uint8_t>(weakest_status),
            static_cast<std::uint8_t>(observation.status)));
        evidence_confidence = std::min(evidence_confidence, observation.confidence);
        if (index == 0)
            continue;
        const std::int64_t delta = observation.onset.tick - observations[index - 1].onset.tick;
        if (delta <= 0)
            throw std::invalid_argument("motif observations must have strictly increasing onset times");
        iois.push_back(static_cast<double>(delta));
    }

    const double median_ioi = median_positive(iois);

    part_motif_profile result;
    result.part_id = part_id;
    result.status = weakest_status;
    result.evidence_confidence = evidence_confidence;
    result.source_nodes.reserve(observations.size());
    for (const auto& observation : observations)
        result.source_nodes.push_back(observation.source_node);
    result.normalized_inter_onset_intervals.reserve(iois.size());
    for (double ioi : iois)
        result.normalized_inter_onset_intervals.push_back(ioi / median_ioi);

    bool all_have_pitch = true;
    bool same_pitch_basis = !observations.front().pitch_basis.empty();
    bool same_interval_semantics = !observations.front().interval_semantics.empty();
    const std::string basis = observations.front().pitch_basis;
    const std::string interval_semantics = observations.front().interval_semantics;
    for (const auto& observation : observations) {
        all_have_pitch = all_have_pitch && observation.log2_pitch_coordinate.has_value() &&
            std::isfinite(*observation.log2_pitch_coordinate);
        same_pitch_basis = same_pitch_basis && observation.pitch_basis == basis;
        same_interval_semantics = same_interval_semantics &&
            observation.interval_semantics == interval_semantics;
    }

    // Coordinate differences are only valid when every event in this one
    // profile shares the same native coordinate basis. Once derived, however,
    // the resulting interval semantics may be comparable with another profile
    // produced from a different native representation.
    if (all_have_pitch && same_pitch_basis && same_interval_semantics) {
        std::vector<double> intervals;
        std::vector<std::int8_t> contour;
        intervals.reserve(observations.size() - 1);
        contour.reserve(observations.size() - 1);

        double low = *observations.front().log2_pitch_coordinate;
        double high = low;
        for (std::size_t index = 1; index < observations.size(); ++index) {
            const double previous = *observations[index - 1].log2_pitch_coordinate;
            const double current = *observations[index].log2_pitch_coordinate;
            const double interval = current - previous;
            intervals.push_back(interval);
            contour.push_back(interval > 1e-9 ? 1 : (interval < -1e-9 ? -1 : 0));
            low = std::min(low, current);
            high = std::max(high, current);
        }

        result.interval_octaves = std::move(intervals);
        result.pitch_contour = std::move(contour);
        result.pitch_basis = basis;
        result.interval_semantics = interval_semantics;
        result.pitch_range_octaves = high - low;
    }

    bool all_have_performance_shape = true;
    for (const auto& observation : observations)
        all_have_performance_shape = all_have_performance_shape && observation.performance_shape.has_value();
    if (all_have_performance_shape) {
        std::vector<part_gesture_performance_shape> shapes;
        shapes.reserve(observations.size());
        for (const auto& observation : observations)
            shapes.push_back(*observation.performance_shape);
        result.performance_shapes = std::move(shapes);
    }

    return result;
}

inline double bounded_difference_similarity(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double distance_weight) {
    if (first.size() != second.size() || first.empty())
        return 0.0;
    double total = 0.0;
    for (std::size_t index = 0; index < first.size(); ++index)
        total += std::fabs(first[index] - second[index]);
    const double mean = total / static_cast<double>(first.size());
    return 1.0 / (1.0 + distance_weight * mean);
}

inline double contour_match_similarity(
    const std::vector<std::int8_t>& first,
    const std::vector<std::int8_t>& second) {
    if (first.size() != second.size() || first.empty())
        return 0.0;
    std::size_t matches = 0;
    for (std::size_t index = 0; index < first.size(); ++index)
        matches += first[index] == second[index] ? 1u : 0u;
    return static_cast<double>(matches) / static_cast<double>(first.size());
}

inline double compare_part_gesture_performance_shapes(
    const std::vector<part_gesture_performance_shape>& first,
    const std::vector<part_gesture_performance_shape>& second) {
    if (first.size() != second.size() || first.empty())
        return 0.0;

    std::size_t kind_matches = 0;
    double range_difference = 0.0;
    double net_difference = 0.0;
    double direction_difference = 0.0;
    for (std::size_t index = 0; index < first.size(); ++index) {
        kind_matches += first[index].kind == second[index].kind ? 1u : 0u;
        range_difference += std::fabs(
            first[index].pitch_range_semitones - second[index].pitch_range_semitones);
        net_difference += std::fabs(
            first[index].net_motion_semitones - second[index].net_motion_semitones);
        const auto first_changes = static_cast<double>(first[index].direction_changes);
        const auto second_changes = static_cast<double>(second[index].direction_changes);
        direction_difference += std::fabs(first_changes - second_changes);
    }

    const double count = static_cast<double>(first.size());
    const double kind_similarity = static_cast<double>(kind_matches) / count;
    const double range_similarity = 1.0 / (1.0 + 0.25 * (range_difference / count));
    const double net_similarity = 1.0 / (1.0 + 0.25 * (net_difference / count));
    const double direction_similarity = 1.0 / (1.0 + direction_difference / count);
    return 0.40 * kind_similarity +
           0.25 * range_similarity +
           0.25 * net_similarity +
           0.10 * direction_similarity;
}

inline part_motif_similarity compare_part_motif_profiles(
    const part_motif_profile& first,
    const part_motif_profile& second) {
    part_motif_similarity result;
    result.rhythm_similarity = bounded_difference_similarity(
        first.normalized_inter_onset_intervals,
        second.normalized_inter_onset_intervals,
        1.0);
    result.evidence_confidence = std::min(
        first.evidence_confidence,
        second.evidence_confidence);

    double weighted_sum = 0.35 * result.rhythm_similarity;
    double total_weight = 0.35;

    result.pitch_comparable =
        first.interval_octaves.has_value() && second.interval_octaves.has_value() &&
        first.pitch_contour.has_value() && second.pitch_contour.has_value() &&
        !first.interval_semantics.empty() &&
        first.interval_semantics == second.interval_semantics;

    if (result.pitch_comparable) {
        result.interval_similarity = bounded_difference_similarity(
            *first.interval_octaves,
            *second.interval_octaves,
            4.0);
        result.contour_similarity = contour_match_similarity(
            *first.pitch_contour,
            *second.pitch_contour);
        weighted_sum += 0.45 * *result.interval_similarity;
        weighted_sum += 0.20 * *result.contour_similarity;
        total_weight += 0.65;
    }

    result.performance_shape_comparable =
        first.performance_shapes.has_value() && second.performance_shapes.has_value() &&
        first.performance_shapes->size() == second.performance_shapes->size() &&
        !first.performance_shapes->empty();
    if (result.performance_shape_comparable) {
        result.performance_shape_similarity = compare_part_gesture_performance_shapes(
            *first.performance_shapes,
            *second.performance_shapes);
        weighted_sum += 0.20 * *result.performance_shape_similarity;
        total_weight += 0.20;
    }

    result.combined_similarity = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
    const double structural_identity = result.pitch_comparable || result.performance_shape_comparable
        ? result.combined_similarity
        : std::min(result.combined_similarity, rhythm_only_motif_identity_ceiling);
    result.identity_confidence = std::min(structural_identity, result.evidence_confidence);
    return result;
}

} // namespace vgmtooling::model
