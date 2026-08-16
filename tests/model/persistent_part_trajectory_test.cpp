#include "model/persistent_part_trajectory.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_episode(musical_execution_graph& graph, std::int64_t start, const char* label) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = label;
    value.active = time_span{
        {time_domain::device, start, 32000, 0},
        time_coordinate{time_domain::device, start + 100, 32000, 0},
    };
    return graph.add_node(std::move(value));
}

persistent_part_hypothesis strong_link(node_id first, node_id second, const char* source) {
    return make_persistent_part_hypothesis(
        0.88,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.94,
                source,
                "same source/program identity",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.84,
                source,
                "adjacent bounded observations",
                {first, second},
            },
        });
}

persistent_part_hypothesis weak_link(node_id first, node_id second) {
    return make_persistent_part_hypothesis(
        0.95,
        {first, second},
        {{
            persistent_part_evidence_kind::physical_slot_continuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::derived,
            0.95,
            "fixture-slot",
            "same physical slot only",
            {first, second},
        }});
}

persistent_part_hypothesis conflicted_link(node_id first, node_id second) {
    return make_persistent_part_hypothesis(
        0.92,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                "fixture-conflict",
                "same source identity",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.82,
                "fixture-conflict",
                "nearby observations",
                {first, second},
            },
            {
                persistent_part_evidence_kind::simultaneous_conflict,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.92,
                "fixture-conflict",
                "observations overlap strongly",
                {first, second},
            },
        });
}

bool throws_invalid(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id a = add_episode(graph, 0, "A");
    const node_id b = add_episode(graph, 120, "B");
    const node_id c = add_episode(graph, 240, "C");
    const node_id d = add_episode(graph, 360, "D");
    const node_id e = add_episode(graph, 480, "E");

    const auto ab = strong_link(a, b, "fixture-ab");
    const auto bc = strong_link(b, c, "fixture-bc");
    const auto cd_reversed = strong_link(d, c, "fixture-cd");

    const auto trajectory = make_persistent_part_trajectory({ab, bc, cd_reversed});
    assert(trajectory.subject_nodes.size() == 4);
    assert(trajectory.subject_nodes[0] == a);
    assert(trajectory.subject_nodes[1] == b);
    assert(trajectory.subject_nodes[2] == c);
    assert(trajectory.subject_nodes[3] == d);
    assert(trajectory.confidence >= persistent_part_trajectory_link_threshold);

    const auto combined = persistent_part_hypothesis_from_trajectory(trajectory);
    assert(combined.subject_nodes.size() == 4);
    assert(combined.identity_bearing_support);
    assert(combined.cross_domain_grounded);
    assert(combined.confidence >= persistent_part_trajectory_link_threshold);

    const node_id part = add_persistent_part_trajectory(graph, trajectory);
    const node* part_node = graph.find_node(part);
    assert(part_node != nullptr);
    assert(part_node->kind == node_kind::part);
    assert(graph.edges_to(part, edge_kind::groups_into).size() == 4);

    const auto branch_one = strong_link(d, e, "fixture-de");
    const node_id f = add_episode(graph, 600, "F");
    const auto branch_two = strong_link(d, f, "fixture-df");
    const auto branch = resolve_unique_persistent_part_successor(
        d,
        {branch_one, branch_two},
        {a, b, c, d});
    assert(!branch.successor.has_value());
    assert(branch.ambiguous);
    assert(branch.strong_transition_indices.size() == 2);

    const auto unique = resolve_unique_persistent_part_successor(
        d,
        {branch_one, weak_link(d, f)},
        {a, b, c, d});
    assert(unique.successor.has_value());
    assert(*unique.successor == e);
    assert(!unique.ambiguous);

    const auto used_filtered = resolve_unique_persistent_part_successor(
        d,
        {branch_one},
        {a, b, c, d, e});
    assert(!used_filtered.successor.has_value());
    assert(!used_filtered.ambiguous);

    assert(throws_invalid([&] {
        (void)make_persistent_part_trajectory({ab, weak_link(b, e)});
    }));

    assert(throws_invalid([&] {
        (void)make_persistent_part_trajectory({ab, conflicted_link(b, e)}, 0.40);
    }));

    assert(throws_invalid([&] {
        const auto ca = strong_link(c, a, "fixture-loop");
        (void)make_persistent_part_trajectory({ab, bc, ca});
    }));

    assert(throws_invalid([&] {
        const auto de = strong_link(d, e, "fixture-disconnected");
        (void)make_persistent_part_trajectory({ab, de});
    }));

    return 0;
}
