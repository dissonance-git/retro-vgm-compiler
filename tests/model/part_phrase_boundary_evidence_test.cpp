#include "model/part_phrase_boundary_evidence.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

part_gesture_observation gesture(
    node_id node,
    std::int64_t tick,
    double pitch) {
    return {
        node,
        42,
        {time_domain::source, tick, 0, 0},
        pitch,
        "synthetic-pitch",
        "log2_frequency_ratio_octaves",
    };
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    const std::vector<part_gesture_observation> observations = {
        gesture(1, 0, 0.00),
        gesture(2, 100, 2.0 / 12.0),
        gesture(3, 200, 4.0 / 12.0),
        gesture(4, 400, 1.0 / 12.0),
        gesture(5, 1000, 1.00),
        gesture(6, 1200, 1.0 + 2.0 / 12.0),
        gesture(7, 1400, 1.0 + 4.0 / 12.0),
        gesture(8, 1800, 1.0 + 1.0 / 12.0),
    };

    part_motif_discovery_policy motif_policy;
    motif_policy.min_events = 4;
    motif_policy.max_events = 4;
    motif_policy.min_identity_confidence = 0.99;
    const auto motifs = discover_repeated_part_motifs(observations, motif_policy);
    assert(motifs.size() == 1);

    auto gaps = detect_part_temporal_gap_candidates(observations, 2.0, "gap-control");
    assert(gaps.size() == 2);
    assert(gaps[0].boundary.tick == 1000);
    assert(gaps[1].boundary.tick == 1800);

    auto motif_boundaries = repeated_motif_boundary_candidates(
        observations,
        motifs.front(),
        "motif-control");
    assert(motif_boundaries.size() == 1);
    assert(motif_boundaries.front().boundary.tick == 1000);
    assert(close_enough(
        motif_boundaries.front().evidence.front().confidence,
        motifs.front().similarity.identity_confidence));

    gaps.insert(
        gaps.end(),
        std::make_move_iterator(motif_boundaries.begin()),
        std::make_move_iterator(motif_boundaries.end()));
    auto merged = merge_part_phrase_boundary_candidates(std::move(gaps));
    assert(merged.size() == 2);

    assert(merged[0].boundary.tick == 1000);
    assert(merged[0].evidence.size() == 2);
    const auto strong = make_part_phrase_boundary_hypothesis(merged[0]);
    assert(strong.structural_support);
    assert(strong.cross_domain_grounded);
    assert(strong.confidence >= 0.80);

    assert(merged[1].boundary.tick == 1800);
    assert(merged[1].evidence.size() == 1);
    const auto weak = make_part_phrase_boundary_hypothesis(merged[1]);
    assert(weak.timing_only);
    assert(close_enough(weak.confidence, phrase_boundary_timing_only_ceiling));

    // Motif structure can strengthen the same location without turning every
    // large timing gap into a phrase boundary.
    assert(strong.confidence > weak.confidence);

    return 0;
}
