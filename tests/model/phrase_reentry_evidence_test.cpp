#include "model/phrase_reentry_evidence.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
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
    std::int64_t tick) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_structure;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{at(tick), at(tick)};
    return graph.add_node(std::move(value));
}

phrase_boundary_evidence boundary_evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    double confidence,
    const char* source,
    node_id support_node) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = phrase_boundary_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "phrase re-entry regression evidence";
    result.support_nodes = {support_node};
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const auto role_scope = span(100, 260);
    const auto reentry = at(180);

    const node_id gap = add_timed_node(graph, "gap", 180);
    const node_id motif_close = add_timed_node(graph, "motif close", 178);
    const auto boundary = make_phrase_boundary_hypothesis(
        reentry,
        0.90,
        {
            boundary_evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.84,
                "gap-analysis",
                gap),
            boundary_evidence(
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                0.88,
                "motif-boundary-analysis",
                motif_close),
        });
    assert(boundary.structural_support);
    assert(boundary.cross_domain_grounded);
    assert(close_enough(boundary.confidence, 0.84));

    const node_id onset_a = add_timed_node(graph, "new onset a", 180);
    const node_id onset_b = add_timed_node(graph, "new onset b", 195);
    const auto new_phrase_evidence = make_post_boundary_new_phrase_evidence(
        graph,
        boundary,
        {onset_a, onset_b},
        role_scope,
        phrase_role_formal_scale::local_phrase);
    assert(new_phrase_evidence.role == phrase_role_kind::new_phrase_onset);
    assert(new_phrase_evidence.origin ==
        phrase_role_evidence_origin::phrase_boundary_analysis);
    assert(close_enough(new_phrase_evidence.confidence, 0.84));

    const auto new_phrase = make_phrase_role_hypothesis(
        phrase_role_kind::new_phrase_onset,
        role_scope,
        phrase_role_formal_scale::local_phrase,
        0.90,
        {new_phrase_evidence},
        {phrase_role_kind::continuation});
    assert(new_phrase.support_domains == 1);
    assert(!new_phrase.cross_domain_grounded);
    assert(close_enough(new_phrase.confidence, phrase_role_single_domain_ceiling));
    assert(!new_phrase.role_established);

    const node_id earlier_motif = add_timed_node(graph, "earlier motif", 125);
    const node_id returned_motif = add_timed_node(graph, "returned motif", 205);
    motif_transformation_hypothesis recurrence;
    recurrence.kind = motif_transformation_kind::near_recurrence;
    recurrence.confidence = 0.91;
    recurrence.first_nodes = {earlier_motif};
    recurrence.second_nodes = {returned_motif};

    const auto boundary_return = make_boundary_return_evidence(
        boundary,
        role_scope,
        phrase_role_formal_scale::phrase_group);
    const auto recurrence_return = make_motif_return_recurrence_evidence(
        graph,
        recurrence,
        boundary,
        role_scope,
        phrase_role_formal_scale::phrase_group);
    assert(boundary_return.origin ==
        phrase_role_evidence_origin::phrase_boundary_analysis);
    assert(recurrence_return.origin ==
        phrase_role_evidence_origin::recurrence_analysis);
    assert(close_enough(recurrence_return.confidence, 0.84));

    const auto return_candidate = make_phrase_role_hypothesis(
        phrase_role_kind::return_role,
        role_scope,
        phrase_role_formal_scale::phrase_group,
        0.92,
        {boundary_return, recurrence_return},
        {phrase_role_kind::continuation, phrase_role_kind::new_phrase_onset});
    assert(return_candidate.support_domains == 2);
    assert(return_candidate.cross_domain_grounded);
    assert(close_enough(return_candidate.confidence, 0.84));
    assert(!return_candidate.role_established);

    const node_id return_node = add_phrase_role_hypothesis(
        graph, return_candidate);
    assert(graph.edges_to(return_node, edge_kind::derived_from).size() == 4);

    // Recurrence alone is not a formal return. A timing-only boundary is also
    // insufficient because it has no structural evidence for phrase re-entry.
    const node_id timing_only = add_timed_node(graph, "timing only", 180);
    const auto weak_boundary = make_phrase_boundary_hypothesis(
        reentry,
        0.90,
        {
            boundary_evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.90,
                "timing-only",
                timing_only),
        });
    assert(weak_boundary.timing_only);

    bool recurrence_without_boundary_rejected = false;
    try {
        (void)make_motif_return_recurrence_evidence(
            graph,
            recurrence,
            weak_boundary,
            role_scope,
            phrase_role_formal_scale::phrase_group);
    } catch (const std::invalid_argument&) {
        recurrence_without_boundary_rejected = true;
    }
    assert(recurrence_without_boundary_rejected);

    // Rhythm alone does not identify returning material.
    bool rhythm_only_rejected = false;
    try {
        auto rhythm_only = recurrence;
        rhythm_only.kind = motif_transformation_kind::rhythm_only_echo;
        (void)make_motif_return_recurrence_evidence(
            graph,
            rhythm_only,
            boundary,
            role_scope,
            phrase_role_formal_scale::phrase_group);
    } catch (const std::invalid_argument&) {
        rhythm_only_rejected = true;
    }
    assert(rhythm_only_rejected);

    // The returned occurrence must actually begin after the grounded boundary.
    bool pre_boundary_return_rejected = false;
    try {
        auto misplaced = recurrence;
        misplaced.second_nodes = {motif_close};
        (void)make_motif_return_recurrence_evidence(
            graph,
            misplaced,
            boundary,
            role_scope,
            phrase_role_formal_scale::phrase_group);
    } catch (const std::invalid_argument&) {
        pre_boundary_return_rejected = true;
    }
    assert(pre_boundary_return_rejected);

    // New-phrase onset likewise requires material at or after the boundary.
    bool early_onset_rejected = false;
    try {
        (void)make_post_boundary_new_phrase_evidence(
            graph,
            boundary,
            {earlier_motif},
            role_scope,
            phrase_role_formal_scale::local_phrase);
    } catch (const std::invalid_argument&) {
        early_onset_rejected = true;
    }
    assert(early_onset_rejected);

    return 0;
}
