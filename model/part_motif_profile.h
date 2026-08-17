#pragma once

#include "musical_execution_graph.h"

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

struct part_gesture_observation {
    node_id source_node = 0;
    node_id part_id = 0;
    time_coordinate onset{};
    std::optional<double> log2_pitch_coordinate{};
    std::string pitch_basis;
    std::string interval_semantics;
    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
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
    evidence_status status = evidence_status::derived;
    double evidence_confidence = 1.0;
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
    double evidence_confidence = 1.0;
};

constexpr double rhythm_only_motif_identity_ceiling = 0.55;

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
    if (observations.size() < 2)
        throw std::invalid_argument("part motif profile requires at least two gestures");

    part_motif_profile result;
    result.part_id = observations.front().part_id;
    result.status = observations.front().status;
    result.evidence_confidence = observations.front().confidence;

    for (const auto& observation : observations) {
        if (observation.part_id != result.part_id)
            throw std::invalid_argument("part motif observations must belong to one persistent part");
        result.source_nodes.push_back(observation.source_node);
        result.status = weaker_part_motif_evidence_status(result.status, observation.status);
        result.evidence_confidence = std::min(result.evidence_confidence, observation.confidence);
    }

    std::vector<double> positive_intervals;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        const auto& previous = observations[index - 1].onset;
        const auto& current = observations[index].onset;
        if (previous.domain != current.domain || previous.tick_rate != current.tick_rate)
            throw std::invalid_argument("part motif timing requires one time domain and tick rate");
        const auto delta = current.tick - previous.tick;
        if (delta <= 0)
            throw std::invalid_argument("part motif observations must be strictly ordered in time");
        positive_intervals.push_back(static_cast<double>(delta));
    }

    const double timing_scale = median_positive(positive_intervals);
    result.normalized_inter_onset_intervals.reserve(positive_intervals.size());
    for (double interval : positive_intervals)
        result.normalized_inter_onset_intervals.push_back(interval / timing_scale);

    bool all_pitched = true;
    std::string interval_semantics;
    std::vector<double> pitches;
    for (const auto& observation : observations) {
        if (!observation.log2_pitch_coordinate.has_value()) {
            all_pitched = false;
            break;
        }
        if (interval_semantics.empty())
            interval_semantics = observation.interval_semantics;
        if (observation.interval_semantics.empty() ||
            observation.interval_semantics != interval_semantics) {
            all_pitched = false;
            break;
        }
        pitches.push_back(*observation.log2_pitch_coordinate);
    }

    if (all_pitched && pitches.size() == observations.size()) {
        result.interval_semantics = interval_semantics;
        result.pitch_basis = observations.front().pitch_basis;
        bool same_basis = !result.pitch_basis.empty();
        for (const auto& observation : observations) {
            if (observation.pitch_basis != result.pitch_basis) {
                same_basis = false;
                break;
            }
        }
        if (!same_basis)
            result.pitch_basis.clear();

        std::vector<double> intervals;
        std::vector<std::int8_t> contour;
        intervals.reserve(pitches.size() - 1);
        contour.reserve(pitches.size() - 1);
        for (std::size_t index = 1; index < pitches.size(); ++index) {
            const double delta = pitches[index] - pitches[index - 1];
            intervals.push_back(delta);
            contour.push_back(delta > 0.0 ? 1 : (delta < 0.0 ? -1 : 0));
        }
        result.interval_octaves = std::move(intervals);
        result.pitch_contour = std::move(contour);
        const auto [minimum, maximum] = std::minmax_element(pitches.begin(), pitches.end());
        result.pitch_range_octaves = *maximum - *minimum;
    }

    return result;
}

inline double bounded_vector_similarity(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double weight) {
    if (first.size() != second.size() || first.empty())
        return 0.0;
    double sum = 0.0;
    for (std::size_t index = 0; index < first.size(); ++index)
        sum += std::fabs(first[index] - second[index]);
    const double mean = sum / static_cast<double>(first.size());
    return 1.0 / (1.0 + weight * mean);
}

inline double contour_similarity(
    const std::vector<std::int8_t>& first,
    const std::vector<std::int8_t>& second) {
    if (first.size() != second.size() || first.empty())
        return 0.0;
    std::size_t same = 0;
    for (std::size_t index = 0; index < first.size(); ++index) {
        if (first[index] == second[index])
            ++same;
    }
    return static_cast<double>(same) / static_cast<double>(first.size());
}

inline part_motif_similarity compare_part_motif_profiles(
    const part_motif_profile& first,
    const part_motif_profile& second) {
    part_motif_similarity result;
    result.rhythm_similarity = bounded_vector_similarity(
        first.normalized_inter_onset_intervals,
        second.normalized_inter_onset_intervals,
        1.0);
    result.evidence_confidence = std::min(
        first.evidence_confidence,
        second.evidence_confidence);

    result.pitch_comparable =
        first.interval_octaves.has_value() &&
        second.interval_octaves.has_value() &&
        first.pitch_contour.has_value() &&
        second.pitch_contour.has_value() &&
        !first.interval_semantics.empty() &&
        first.interval_semantics == second.interval_semantics;

    if (result.pitch_comparable) {
        result.interval_similarity = bounded_vector_similarity(
            *first.interval_octaves,
            *second.interval_octaves,
            4.0);
        result.contour_similarity = contour_similarity(
            *first.pitch_contour,
            *second.pitch_contour);
        result.combined_similarity =
            0.35 * result.rhythm_similarity +
            0.45 * *result.interval_similarity +
            0.20 * *result.contour_similarity;
        result.identity_confidence = std::min(
            result.combined_similarity,
            result.evidence_confidence);
    } else {
        result.combined_similarity = result.rhythm_similarity;
        result.identity_confidence = std::min({
            result.rhythm_similarity,
            result.evidence_confidence,
            rhythm_only_motif_identity_ceiling,
        });
    }
    return result;
}

} // namespace vgmtooling::model
