#include "model/phrase_boundary_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

node_id add_event(musical_execution_graph& graph, std::int64_t tick, const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{{time_domain::source, tick, 0, 0}, std::nullopt};
    return graph.add_node(std::move(value));
}

phrase_boundary_evidence evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    double confidence,
    const char* source,
    node_id support_node,
    phrase_boundary_evidence_polarity polarity = phrase_boundary_evidence_polarity::supports,
    evidence_status status = evidence_status::derived) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = polarity;
    result.status = status;
    result.confidence = confidence;
    result.source = source;
    result.detail = "synthetic phrase-boundary evidence";
    if (support_node != 0)
        result.support_nodes.push_back(support_node);
    return result;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id before = add_event(graph, 900, "event before boundary");
    const node_id at = add_event(graph, 1000, "event at boundary");
    const time_coordinate boundary{time_domain::source, 1000, 0, 0};

    // A conspicuous rest is evidence, not phrase truth.
    const auto timing_only = make_phrase_boundary_hypothesis(
        boundary,
        0.95,
        {evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.95,
            "gap-detector",
            at)});
    assert(timing_only.timing_only);
    assert(!timing_only.structural_support);
    assert(!timing_only.cross_domain_grounded);
    assert(close_enough(timing_only.confidence, phrase_boundary_timing_only_ceiling));

    // A driver loop plus a gap still lacks structural musical evidence.
    const auto implementation_boundary = make_phrase_boundary_hypothesis(
        boundary,
        0.94,
        {
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.92,
                "gap-detector",
                at),
            evidence(
                phrase_boundary_evidence_kind::driver_loop_boundary,
                phrase_boundary_evidence_origin::driver_execution,
                0.91,
                "driver-loop",
                at),
        });
    assert(!implementation_boundary.timing_only);
    assert(!implementation_boundary.structural_support);
    assert(implementation_boundary.cross_domain_grounded);
    assert(close_enough(
        implementation_boundary.confidence,
        phrase_boundary_nonstructural_ceiling));

    // Motif completion is structural, but one analytical domain alone remains
    // below a strong phrase-boundary threshold.
    const auto motif_only = make_phrase_boundary_hypothesis(
        boundary,
        0.92,
        {evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            0.90,
            "motif-analysis",
            before)});
    assert(motif_only.structural_support);
    assert(!motif_only.cross_domain_grounded);
    assert(close_enough(motif_only.confidence, phrase_boundary_single_domain_ceiling));

    // Timing plus independently derived motif completion can establish a strong
    // boundary without requiring chord/cadence inference to exist yet.
    const auto convergent = make_phrase_boundary_hypothesis(
        boundary,
        0.94,
        {
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.91,
                "gap-detector",
                at),
            evidence(
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                0.88,
                "motif-analysis",
                before),
        });
    assert(convergent.structural_support);
    assert(convergent.cross_domain_grounded);
    assert(close_enough(convergent.independent_support_ceiling, 0.88));
    assert(close_enough(convergent.confidence, 0.88));

    // Strong continuity across the proposed boundary should prevent a confident
    // segmentation unless exact authored evidence says otherwise.
    const auto conflicted = make_phrase_boundary_hypothesis(
        boundary,
        0.94,
        {
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.91,
                "gap-detector",
                at),
            evidence(
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                0.88,
                "motif-analysis",
                before),
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                0.93,
                "continuity-test",
                at,
                phrase_boundary_evidence_polarity::counters),
        });
    assert(conflicted.strong_conflict_present);
    assert(close_enough(conflicted.confidence, phrase_boundary_strong_conflict_ceiling));

    // Exact authored phrase markers can remain strong in one evidence domain.
    const auto authored = make_phrase_boundary_hypothesis(
        boundary,
        0.96,
        {evidence(
            phrase_boundary_evidence_kind::authored_boundary,
            phrase_boundary_evidence_origin::authored_source,
            0.97,
            "source-marker",
            at,
            phrase_boundary_evidence_polarity::supports,
            evidence_status::exact)});
    assert(authored.authored_grounded);
    assert(close_enough(authored.confidence, 0.96));

    const node_id boundary_node = add_phrase_boundary_hypothesis(graph, convergent);
    const node* materialized = graph.find_node(boundary_node);
    assert(materialized != nullptr);
    assert(materialized->kind == node_kind::musical_relation);
    assert(materialized->layer == semantic_layer::musical_structure);
    assert(materialized->active.has_value());
    assert(materialized->active->start.tick == 1000);
    assert(graph.edges_to(boundary_node, edge_kind::derived_from).size() == 2);

    return 0;
}
