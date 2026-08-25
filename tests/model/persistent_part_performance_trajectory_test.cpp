#include "model/persistent_part_performance_trajectory.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

persistent_part_hypothesis strong_link(node_id first, node_id second, double confidence) {
    return make_persistent_part_hypothesis(
        confidence,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                "performance-trajectory-fixture",
                "same source identity across adjacent physical episodes",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.90,
                "performance-trajectory-fixture",
                "bounded physical episodes are adjacent",
                {first, second},
            },
        });
}

std::vector<pitch_motion_sample> samples(
    node_id episode,
    node_id source_base,
    const std::vector<double>& semitone_offsets) {
    std::vector<pitch_motion_sample> result;
    for (std::size_t index = 0; index < semitone_offsets.size(); ++index) {
        result.push_back({
            static_cast<node_id>(source_base + index),
            episode,
            time_coordinate{
                time_domain::source,
                static_cast<std::int64_t>(index * 10),
                0,
                0,
            },
            semitone_offsets[index] / 12.0,
            "shared-log2-frequency",
            "log2_frequency_ratio_octaves",
        });
    }
    return result;
}

bool throws_invalid(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    const auto identity = make_persistent_part_trajectory({
        strong_link(10, 20, 0.91),
        strong_link(20, 30, 0.88),
    });

    const auto first = make_persistent_part_performance_segment(
        samples(10, 100, {0.0, 0.0}),
        0.94);
    const auto second = make_persistent_part_performance_segment(
        samples(20, 200, {0.0, 0.5, 1.0, 1.5}),
        0.86);
    const auto third = make_persistent_part_performance_segment(
        samples(30, 300, {0.0, 0.5, -0.5, 0.5, 0.0}),
        0.92);

    assert(first.articulation.kind == pitch_motion_articulation_kind::steady_pitch);
    assert(second.articulation.kind == pitch_motion_articulation_kind::glide_candidate);
    assert(third.articulation.kind == pitch_motion_articulation_kind::periodic_modulation_candidate);

    const auto boundary_one = make_rearticulation_boundary(10, 20, 0.95);
    const auto boundary_two = make_rearticulation_boundary(20, 30, 0.84);
    const auto performance = make_persistent_part_performance_trajectory(
        identity,
        {first, second, third},
        {boundary_one, boundary_two});

    assert(performance.identity.subject_nodes.size() == 3);
    assert(performance.segments.size() == 3);
    assert(performance.rearticulation_boundaries.size() == 2);
    assert(performance.segments[1].physical_episode_id == 20);
    assert(performance.segments[1].samples.size() == 4);
    assert(performance.rearticulation_boundaries[0].physical_episode_id == 10);
    assert(performance.rearticulation_boundaries[0].next_physical_episode_id == 20);
    assert(close_enough(performance.confidence, 0.84));

    assert(throws_invalid([&] {
        (void)make_persistent_part_performance_trajectory(
            identity,
            {second, first, third},
            {boundary_one, boundary_two});
    }));

    assert(throws_invalid([&] {
        (void)make_persistent_part_performance_trajectory(
            identity,
            {first, second, third},
            {make_rearticulation_boundary(10, 30, 0.95), boundary_two});
    }));

    assert(throws_invalid([&] {
        auto malformed = first;
        malformed.articulation.next_physical_episode_id = 20;
        (void)make_persistent_part_performance_trajectory(
            identity,
            {malformed, second, third},
            {boundary_one, boundary_two});
    }));

    assert(throws_invalid([&] {
        auto malformed = second;
        malformed.samples.front().source_node += 1000;
        (void)make_persistent_part_performance_trajectory(
            identity,
            {first, malformed, third},
            {boundary_one, boundary_two});
    }));

    return 0;
}
