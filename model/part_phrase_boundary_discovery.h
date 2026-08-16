#pragma once

#include "part_phrase_boundary_evidence.h"

#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

inline std::vector<phrase_boundary_hypothesis> discover_part_phrase_boundaries(
    const std::vector<part_gesture_observation>& observations,
    const part_motif_discovery_policy& motif_policy = {},
    double minimum_gap_ratio = 2.0,
    double proposed_confidence = 0.95,
    std::string source_prefix = "part-phrase-discovery") {
    if (source_prefix.empty())
        throw std::invalid_argument("part phrase discovery requires a source prefix");

    auto candidates = detect_part_temporal_gap_candidates(
        observations,
        minimum_gap_ratio,
        source_prefix + ":timing");

    const auto motifs = discover_repeated_part_motifs(observations, motif_policy);
    for (const auto& motif : motifs) {
        auto motif_candidates = repeated_motif_boundary_candidates(
            observations,
            motif,
            source_prefix + ":motif");
        candidates.insert(
            candidates.end(),
            std::make_move_iterator(motif_candidates.begin()),
            std::make_move_iterator(motif_candidates.end()));
    }

    auto merged = merge_part_phrase_boundary_candidates(std::move(candidates));
    std::vector<phrase_boundary_hypothesis> hypotheses;
    hypotheses.reserve(merged.size());
    for (auto& candidate : merged) {
        hypotheses.push_back(make_part_phrase_boundary_hypothesis(
            std::move(candidate),
            proposed_confidence));
    }
    return hypotheses;
}

} // namespace vgmtooling::model
