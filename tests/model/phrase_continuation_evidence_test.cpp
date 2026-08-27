#include "model/phrase_continuation_evidence.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 1000, 0};
}

time_span span(std::int64_t start, std::int64_t end) {
    return {at(start), at(end)};
}

node_id add_timed_node(
    musical_execution_graph& graph,
    const char* label,
    std::int64_t start,
    std::int64_t end) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{at(start), at(end)};
    return graph.add_node(std::move(value));
}

persistent_part_evidence part_evidence(
    persistent_part_evidence_kind kind,
    persistent_part_evidence_origin origin,
    double confidence,
    const char* source,
    std::vector<node_id> support_nodes) {
    persistent_part_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = persistent_part_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "persistent-part continuation regression evidence";
    result.support_nodes = std::move(support_nodes);
    return result;
}

phrase_boundary_evidence boundary_evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    phrase_boundary_evidence_polarity polarity,
    double confidence,
    const char* source,
    node_id support_node) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = polarity;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "phrase-boundary continuation regression evidence";
    result.support_nodes = {support_node};
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const auto role_scope = span(100, 220);
    const auto arrival = at(150);

    const node_id part_before = add_timed_node(graph, "part before", 110, 110);
    const node_id part_near = add_timed_node(graph, "part near arrival", 140, 140);
    const node_id part_after = add_timed_node(graph, "part after", 190, 190);

    const auto part_link_a = make_persistent_part_hypothesis(
        0.90,
        {part_before, part_near},
        {
            part_evidence(
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::authored_program,
                0.94,
                "part-source-identity-a",
                {part_before, part_near}),
            part_evidence(
                persistent_part_evidence_kind::pitch_trajectory_continuity,
                persistent_part_evidence_origin::musical_analysis,
                0.90,
                "part-pitch-continuity-a",
                {part_before, part_near}),
        });
    const auto part_link_b = make_persistent_part_hypothesis(
        0.88,
        {part_near, part_after},
        {
            part_evidence(
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::authored_program,
                0.92,
                "part-source-identity-b",
                {part_near, part_after}),
            part_evidence(
                persistent_part_evidence_kind::pitch_trajectory_continuity,
                persistent_part_evidence_origin::musical_analysis,
                0.88,
                "part-pitch-continuity-b",
                {part_near, part_after}),
        });
    const auto trajectory = make_persistent_part_trajectory(
        {part_link_a, part_link_b});
    const auto part_continuation = make_persistent_part_continuation_evidence(
        graph,
        trajectory,
        arrival,
        role_scope,
        phrase_role_formal_scale::local_phrase);
    assert(part_continuation.origin ==
        phrase_role_evidence_origin::persistent_part_continuation);
    assert(close_enough(part_continuation.confidence, 0.88));
    assert(part_continuation.support_nodes.size() == 3);

    const node_id motif_first = add_timed_node(graph, "motif first", 135, 135);
    const node_id motif_second = add_timed_node(graph, "motif second", 170, 170);
    motif_transformation_hypothesis motif;
    motif.kind = motif_transformation_kind::transformed_relation;
    motif.confidence = 0.86;
    motif.first_nodes = {motif_first};
    motif.second_nodes = {motif_second};

    const auto motif_continuation = make_motif_sequence_continuation_evidence(
        graph,
        motif,
        arrival,
        role_scope,
        phrase_role_formal_scale::local_phrase);
    assert(motif_continuation.origin ==
        phrase_role_evidence_origin::motif_analysis);
    assert(close_enough(motif_continuation.confidence, 0.86));

    const node_id harmony_first = add_timed_node(graph, "harmony first", 120, 120);
    const node_id harmony_middle = add_timed_node(graph, "harmony middle", 150, 150);
    const node_id harmony_last = add_timed_node(graph, "harmony last", 190, 190);

    harmonic_transition_hypothesis first_transition;
    first_transition.first_time = at(120);
    first_transition.second_time = at(150);
    first_transition.first_root_pitch_class = 7;
    first_transition.second_root_pitch_class = 9;
    first_transition.first_quality = tertian_triad_quality::major;
    first_transition.second_quality = tertian_triad_quality::minor;
    first_transition.root_motion_reliable = true;
    first_transition.confidence = 0.83;

    harmonic_transition_hypothesis second_transition;
    second_transition.first_time = at(150);
    second_transition.second_time = at(190);
    second_transition.first_root_pitch_class = 9;
    second_transition.second_root_pitch_class = 2;
    second_transition.first_quality = tertian_triad_quality::minor;
    second_transition.second_quality = tertian_triad_quality::minor;
    second_transition.root_motion_reliable = true;
    second_transition.confidence = 0.80;

    const std::vector<harmonic_transition_hypothesis> harmonic_chain{
        first_transition,
        second_transition,
    };
    const auto harmonic_continuation = make_harmonic_process_continuation_evidence(
        graph,
        harmonic_chain,
        {harmony_first, harmony_middle, harmony_last},
        arrival,
        role_scope,
        phrase_role_formal_scale::local_phrase);
    assert(harmonic_continuation.origin ==
        phrase_role_evidence_origin::harmonic_process);
    assert(close_enough(harmonic_continuation.confidence, 0.80));

    const node_id continuity_node = add_timed_node(
        graph, "cross-boundary continuity", 155, 155);
    const node_id gap_node = add_timed_node(graph, "local temporal gap", 150, 150);
    const node_id cadence_node = add_timed_node(graph, "cadence-like arrival", 150, 150);
    const auto boundary = make_phrase_boundary_hypothesis(
        arrival,
        0.92,
        {
            boundary_evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                0.76,
                "gap-analysis",
                gap_node),
            boundary_evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.87,
                "continuity-analysis",
                continuity_node),
            boundary_evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.95,
                "cadence-analysis",
                cadence_node),
        });

    const auto boundary_projection = project_phrase_boundary_continuation_evidence(
        boundary,
        role_scope,
        phrase_role_formal_scale::local_phrase);
    assert(boundary_projection.cross_boundary_continuity_grounded);
    assert(boundary_projection.cadence_derived_evidence_present);
    assert(boundary_projection.noncontinuity_boundary_observations_ignored == 1);
    assert(boundary_projection.evidence.size() == 1);
    assert(boundary_projection.evidence.front().origin ==
        phrase_role_evidence_origin::phrase_boundary_analysis);
    assert(close_enough(boundary_projection.evidence.front().confidence, 0.87));

    std::vector<phrase_role_evidence> continuation_evidence{
        part_continuation,
        motif_continuation,
        harmonic_continuation,
    };
    continuation_evidence.insert(
        continuation_evidence.end(),
        boundary_projection.evidence.begin(),
        boundary_projection.evidence.end());

    const auto continuation = make_phrase_role_hypothesis(
        phrase_role_kind::continuation,
        role_scope,
        phrase_role_formal_scale::local_phrase,
        0.92,
        std::move(continuation_evidence),
        {phrase_role_kind::ending});
    assert(continuation.support_domains == 4);
    assert(continuation.supporting_observations == 4);
    assert(continuation.counter_observations == 0);
    assert(continuation.cross_domain_grounded);
    assert(close_enough(continuation.independent_support_ceiling, 0.87));
    assert(close_enough(continuation.confidence, 0.87));
    assert(!continuation.role_established);

    const node_id continuation_node = add_phrase_role_hypothesis(
        graph, continuation);
    assert(graph.edges_to(
        continuation_node,
        edge_kind::derived_from).size() == 9);

    // A persistent-part relation confined to the pre-arrival side cannot be
    // promoted into continuation evidence.
    bool one_sided_part_rejected = false;
    try {
        const auto early_trajectory = make_persistent_part_trajectory(
            {part_link_a});
        (void)make_persistent_part_continuation_evidence(
            graph,
            early_trajectory,
            arrival,
            role_scope,
            phrase_role_formal_scale::local_phrase);
    } catch (const std::invalid_argument&) {
        one_sided_part_rejected = true;
    }
    assert(one_sided_part_rejected);

    // Weak motif resemblance is not a sequence witness.
    bool weak_motif_rejected = false;
    try {
        auto weak = motif;
        weak.kind = motif_transformation_kind::weak_relation;
        (void)make_motif_sequence_continuation_evidence(
            graph,
            weak,
            arrival,
            role_scope,
            phrase_role_formal_scale::local_phrase);
    } catch (const std::invalid_argument&) {
        weak_motif_rejected = true;
    }
    assert(weak_motif_rejected);

    // Harmonic evidence must describe one continuous process, not two unrelated
    // transitions placed on opposite sides of the arrival.
    bool broken_harmony_rejected = false;
    try {
        auto broken = harmonic_chain;
        broken[1].first_time = at(160);
        (void)make_harmonic_process_continuation_evidence(
            graph,
            broken,
            {harmony_first, harmony_middle, harmony_last},
            arrival,
            role_scope,
            phrase_role_formal_scale::local_phrase);
    } catch (const std::invalid_argument&) {
        broken_harmony_rejected = true;
    }
    assert(broken_harmony_rejected);

    // Cadence-derived boundary evidence remains visible as excluded evidence but
    // can never bootstrap a continuation role by itself.
    const auto cadence_only_boundary = make_phrase_boundary_hypothesis(
        arrival,
        0.90,
        {
            boundary_evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.95,
                "cadence-only",
                cadence_node),
        });
    const auto cadence_only_projection =
        project_phrase_boundary_continuation_evidence(
            cadence_only_boundary,
            role_scope,
            phrase_role_formal_scale::local_phrase);
    assert(cadence_only_projection.cadence_derived_evidence_present);
    assert(!cadence_only_projection.cross_boundary_continuity_grounded);
    assert(cadence_only_projection.evidence.empty());

    return 0;
}
