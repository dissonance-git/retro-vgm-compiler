#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct part_gesture_observation {
    node_id source_node = 0;
    node_id part_id = 0;
    time_coordinate onset{};
    std::optional<double> log2_pitch_coordinate{};
    std::string pitch_basis;
};

struct part_motif_profile {
    node_id part_id = 0;
    std::vector<node_id> source_nodes;
    std::vector<double> normalized_inter_onset_intervals;
    std::optional<std::vector<double>> interval_octaves{};
    std::optional<std::vector<std::int8_t>> pitch_contour{};
    std::string pitch_basis;
    std::optional<double> pitch_range_octaves{};
};

struct part_motif_similarity {
    std::optional<double> interval_similarity{};
    double rhythm_similarity = 0.0;
    std::optional<double> contour_similarity{};
    double combined_similarity = 0.0;
    double identity_confidence = 0.0;
    bool pitch_comparable = false;
    bool transposition_invariant = true;
    bool tempo_scale_invariant = true;
};

constexpr double rhythm_only_motif_identity_ceiling = 0.55;

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
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const auto& observation = observations[index];
        if (observation.part_id != part_id)
            throw std::invalid_argument("one motif profile cannot silently merge different persistent parts");
        if (observation.onset.domain != time_domain_value ||
            observation.onset.tick_rate != tick_rate ||
            observation.onset.loop_iteration != loop_iteration) {
            throw std::invalid_argument("motif observations require one compatible local time and loop basis");
        }
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
    result.source_nodes.reserve(observations.size());
    for (const auto& observation : observations)
        result.source_nodes.push_back(observation.source_node);
    result.normalized_inter_onset_intervals.reserve(iois.size());
    for (double ioi : iois)
        result.normalized_inter_onset_intervals.push_back(ioi / median_ioi);

    bool all_have_pitch = true;
    bool same_pitch_basis = !observations.front().pitch_basis.empty();
    const std::string basis = observations.front().pitch_basis;
    for (const auto& observation : observations) {
        all_have_pitch = all_have_pitch && observation.log2_pitch_coordinate.has_value() &&
            std::isfinite(*observation.log2_pitch_coordinate);
        same_pitch_basis = same_pitch_basis && observation.pitch_basis == basis;
    }

    if (all_have_pitch && same_pitch_basis) {
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
        result.pitch_range_octaves = high - low;
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

inline part_motif_similarity compare_part_motif_profiles(
    const part_motif_profile& first,
    const part_motif_profile& second) {
    part_motif_similarity result;
    result.rhythm_similarity = bounded_difference_similarity(
        first.normalized_inter_onset_intervals,
        second.normalized_inter_onset_intervals,
        1.0);

    double weighted_sum = 0.35 * result.rhythm_similarity;
    double total_weight = 0.35;

    result.pitch_comparable =
        first.interval_octaves.has_value() && second.interval_octaves.has_value() &&
        first.pitch_contour.has_value() && second.pitch_contour.has_value() &&
        !first.pitch_basis.empty() && first.pitch_basis == second.pitch_basis;

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

    result.combined_similarity = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
    result.identity_confidence = result.pitch_comparable
        ? result.combined_similarity
        : std::min(result.combined_similarity, rhythm_only_motif_identity_ceiling);
    return result;
}

} // namespace vgmtooling::model
