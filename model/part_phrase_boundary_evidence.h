#pragma once

#include "part_motif_discovery.h"
#include "phrase_boundary_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct part_phrase_boundary_candidate {
    time_coordinate boundary{};
    std::vector<phrase_boundary_evidence> evidence;
};

inline bool same_phrase_boundary_coordinate(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick == second.tick &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration;
}

inline void validate_ordered_part_gestures(
    const std::vector<part_gesture_observation>& observations) {
    if (observations.size() < 2)
        return;
    const node_id part = observations.front().part_id;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const auto& current = observations[index];
        if (current.part_id != part)
            throw std::invalid_argument("phrase evidence cannot merge different persistent parts");
        if (index == 0)
            continue;
        const auto& previous = observations[index - 1];
        if (current.onset.domain != previous.onset.domain ||
            current.onset.tick_rate != previous.onset.tick_rate ||
            current.onset.loop_iteration != previous.onset.loop_iteration) {
            throw std::invalid_argument("phrase evidence requires one compatible local time and loop basis");
        }
        if (current.onset.tick <= previous.onset.tick)
            throw std::invalid_argument("phrase evidence requires strictly increasing onsets");
    }
}

inline std::vector<part_phrase_boundary_candidate> detect_part_temporal_gap_candidates(
    const std::vector<part_gesture_observation>& observations,
    double minimum_gap_ratio = 2.0,
    std::string source = "part-temporal-gap-analysis") {
    if (minimum_gap_ratio <= 1.0 || !std::isfinite(minimum_gap_ratio))
        throw std::invalid_argument("phrase gap ratio must be finite and greater than one");
    if (source.empty())
        throw std::invalid_argument("phrase gap analysis requires a source");
    validate_ordered_part_gestures(observations);
    if (observations.size() < 3)
        return {};

    std::vector<double> iois;
    iois.reserve(observations.size() - 1);
    for (std::size_t index = 1; index < observations.size(); ++index) {
        iois.push_back(static_cast<double>(
            observations[index].onset.tick - observations[index - 1].onset.tick));
    }
    const double local_median = median_positive(iois);

    std::vector<part_phrase_boundary_candidate> results;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        const double gap = static_cast<double>(
            observations[index].onset.tick - observations[index - 1].onset.tick);
        const double ratio = gap / local_median;
        if (ratio < minimum_gap_ratio)
            continue;

        const double evidence_confidence = std::min(
            0.95,
            0.80 + 0.05 * (ratio - minimum_gap_ratio));
        phrase_boundary_evidence item;
        item.kind = phrase_boundary_evidence_kind::temporal_gap;
        item.origin = phrase_boundary_evidence_origin::performance_timing;
        item.polarity = phrase_boundary_evidence_polarity::supports;
        item.status = evidence_status::derived;
        item.confidence = evidence_confidence;
        item.source = source;
        item.detail =
            "inter-onset gap is " + std::to_string(ratio) +
            "x the local median IOI";
        item.support_nodes = {
            observations[index - 1].source_node,
            observations[index].source_node,
        };
        results.push_back({observations[index].onset, {std::move(item)}});
    }
    return results;
}

inline std::vector<part_phrase_boundary_candidate> repeated_motif_boundary_candidates(
    const std::vector<part_gesture_observation>& observations,
    const repeated_part_motif_hypothesis& motif,
    std::string source = "part-repeated-motif-analysis") {
    if (source.empty())
        throw std::invalid_argument("repeated-motif phrase analysis requires a source");
    validate_ordered_part_gestures(observations);

    std::vector<part_phrase_boundary_candidate> results;
    for (const part_motif_window* window : {&motif.first, &motif.second}) {
        const std::size_t end = window->start_index + window->event_count;
        if (window->event_count == 0 || end > observations.size())
            throw std::invalid_argument("motif window lies outside the supplied part observations");
        if (end == observations.size())
            continue;

        phrase_boundary_evidence item;
        item.kind = phrase_boundary_evidence_kind::repeated_motif_alignment;
        item.origin = phrase_boundary_evidence_origin::motif_analysis;
        item.polarity = phrase_boundary_evidence_polarity::supports;
        item.status = evidence_status::hypothesis;
        item.confidence = motif.similarity.identity_confidence;
        item.source = source;
        item.detail =
            "candidate boundary follows a repeated motif occurrence with identity confidence " +
            std::to_string(motif.similarity.identity_confidence);
        item.support_nodes = {
            observations[end - 1].source_node,
            observations[end].source_node,
        };
        results.push_back({observations[end].onset, {std::move(item)}});
    }
    return results;
}

inline std::vector<part_phrase_boundary_candidate> merge_part_phrase_boundary_candidates(
    std::vector<part_phrase_boundary_candidate> candidates) {
    std::vector<part_phrase_boundary_candidate> merged;
    for (auto& candidate : candidates) {
        auto existing = std::find_if(
            merged.begin(),
            merged.end(),
            [&](const auto& item) {
                return same_phrase_boundary_coordinate(item.boundary, candidate.boundary);
            });
        if (existing == merged.end()) {
            merged.push_back(std::move(candidate));
        } else {
            existing->evidence.insert(
                existing->evidence.end(),
                std::make_move_iterator(candidate.evidence.begin()),
                std::make_move_iterator(candidate.evidence.end()));
        }
    }

    std::sort(merged.begin(), merged.end(), [](const auto& first, const auto& second) {
        if (first.boundary.loop_iteration != second.boundary.loop_iteration)
            return first.boundary.loop_iteration < second.boundary.loop_iteration;
        return first.boundary.tick < second.boundary.tick;
    });
    return merged;
}

inline phrase_boundary_hypothesis make_part_phrase_boundary_hypothesis(
    part_phrase_boundary_candidate candidate,
    double proposed_confidence = 0.95) {
    return make_phrase_boundary_hypothesis(
        candidate.boundary,
        proposed_confidence,
        std::move(candidate.evidence));
}

} // namespace vgmtooling::model
