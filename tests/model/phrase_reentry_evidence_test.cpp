#include "model/phrase_reentry_evidence.h"

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

node_id add_persistent_part(
    musical_execution_graph& graph,
    const char* label,
    double confidence) {
    node value;
    value.kind = node_kind::part;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = label;
    value.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    return graph.add_node(std::move(value));
}

node_id add_performance_event(
    musical_execution_graph& graph,
    const char* label,
    std::int64_t tick) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{at(tick), at(tick)};
    return graph.add_node(std::move(value));
}

part_gesture_observation gesture(
    node_id source,
    node_id part,
    std::int64_t tick,
    double confidence) {
    part_gesture_observation result;
    result.source_node = source;
    result.part_id = part;
    result.onset = at(tick);
    result.status = evidence_status::derived;
    result.confidence = confidence;
    return result;
}

phrase_boundary_consensus consensus(
    std::int64_t tick,
    double confidence,
    const std::vector<node_id>& parts) {
    phrase_boundary_consensus result;
    result.representative = at(tick);
    result.alignment_span = time_span{at(tick), at(tick)};
    result.confidence = confidence;
    result.independent_part_ceiling = confidence;
    result.supporting_parts = parts;
    result.cross_part_grounded = parts.size() >= 2;
    return result;
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


    // The canonical path keeps boundary detection and performed re-onset as
    // independent evidence domains, then requires a materialized phrase-to-
    // phrase recurrence before calling the later region a return.
    {
        musical_execution_graph canonical_graph;
        const node_id melody = add_persistent_part(
            canonical_graph, "canonical melody", 0.92);
        const node_id bass = add_persistent_part(
            canonical_graph, "canonical bass", 0.90);
        const std::vector<node_id> canonical_parts{melody, bass};

        const auto c0 = consensus(0, 0.90, canonical_parts);
        const auto c1 = consensus(400, 0.86, canonical_parts);
        const auto c2 = consensus(800, 0.84, canonical_parts);
        const auto phrase_a = make_phrase_region_hypothesis(c0, c1);
        const auto phrase_b = make_phrase_region_hypothesis(c1, c2);
        const node_id phrase_a_id =
            add_phrase_region_hypothesis(canonical_graph, phrase_a);
        const node_id phrase_b_id =
            add_phrase_region_hypothesis(canonical_graph, phrase_b);

        const node_id canonical_gap =
            add_timed_node(canonical_graph, "canonical gap", 400);
        const node_id canonical_close =
            add_timed_node(canonical_graph, "canonical motif close", 398);
        const auto canonical_boundary = make_phrase_boundary_hypothesis(
            at(400),
            0.92,
            {
                boundary_evidence(
                    phrase_boundary_evidence_kind::temporal_gap,
                    phrase_boundary_evidence_origin::performance_timing,
                    0.88,
                    "canonical-gap",
                    canonical_gap),
                boundary_evidence(
                    phrase_boundary_evidence_kind::motif_completion,
                    phrase_boundary_evidence_origin::motif_analysis,
                    0.86,
                    "canonical-close",
                    canonical_close),
            });
        assert(canonical_boundary.cross_domain_grounded);
        assert(close_enough(canonical_boundary.confidence, 0.86));

        const auto canonical_boundary_evidence =
            make_grounded_reentry_boundary_evidence(
                canonical_boundary,
                phrase_b.span,
                phrase_role_formal_scale::local_phrase);

        const node_id melody_onset = add_performance_event(
            canonical_graph, "canonical melody re-onset", 405);
        const node_id bass_onset = add_performance_event(
            canonical_graph, "canonical bass re-onset", 410);
        const auto performed_reonset =
            make_performed_phrase_reonset_evidence(
                canonical_graph,
                at(400),
                {
                    gesture(melody_onset, melody, 405, 0.91),
                    gesture(bass_onset, bass, 410, 0.88),
                },
                16,
                phrase_b.span,
                phrase_role_formal_scale::local_phrase);
        assert(performed_reonset.origin ==
            phrase_role_evidence_origin::performance_reonset);
        assert(close_enough(performed_reonset.confidence, 0.88));

        const auto grounded_onset =
            make_grounded_new_phrase_onset_hypothesis(
                canonical_boundary_evidence,
                performed_reonset);
        assert(grounded_onset.support_domains == 2);
        assert(grounded_onset.cross_domain_grounded);
        assert(close_enough(grounded_onset.confidence, 0.86));
        assert(!grounded_onset.role_established);

        const node_id a1 = add_performance_event(
            canonical_graph, "canonical A1", 100);
        const node_id a2 = add_performance_event(
            canonical_graph, "canonical A2", 200);
        const node_id a3 = add_performance_event(
            canonical_graph, "canonical A3", 300);
        const node_id b1 = add_performance_event(
            canonical_graph, "canonical B1", 500);
        const node_id b2 = add_performance_event(
            canonical_graph, "canonical B2", 600);
        const node_id b3 = add_performance_event(
            canonical_graph, "canonical B3", 700);

        motif_transformation_hypothesis canonical_recurrence;
        canonical_recurrence.kind =
            motif_transformation_kind::near_recurrence;
        canonical_recurrence.confidence = 0.82;
        canonical_recurrence.first_nodes = {a1, a2, a3};
        canonical_recurrence.second_nodes = {b1, b2, b3};
        const auto canonical_relation = infer_phrase_relation(
            canonical_graph,
            phrase_a_id,
            phrase_b_id,
            canonical_recurrence);
        assert(canonical_relation.kind == phrase_relation_kind::recurrence);
        assert(close_enough(canonical_relation.confidence, 0.82));

        const auto canonical_return =
            make_grounded_phrase_return_hypothesis(
                canonical_graph,
                grounded_onset,
                canonical_relation);
        assert(canonical_return.support_domains == 3);
        assert(canonical_return.cross_domain_grounded);
        assert(close_enough(canonical_return.confidence, 0.82));
        assert(!phrase_role_lists_incompatible(
            canonical_return,
            phrase_role_kind::new_phrase_onset));
        assert(!canonical_return.role_established);

        // A weak recurrence relation is a hard ceiling even when boundary and
        // performed re-onset evidence are stronger.
        auto weak_recurrence = canonical_relation;
        weak_recurrence.confidence = 0.55;
        const auto weak_return =
            make_grounded_phrase_return_hypothesis(
                canonical_graph,
                grounded_onset,
                weak_recurrence);
        assert(close_enough(weak_return.confidence, 0.55));

        // Boundary-only new-phrase evidence remains a useful candidate but
        // cannot be promoted through the canonical return helper.
        const auto boundary_only = make_phrase_role_hypothesis(
            phrase_role_kind::new_phrase_onset,
            phrase_b.span,
            phrase_role_formal_scale::local_phrase,
            0.95,
            {canonical_boundary_evidence});
        assert(!boundary_only.cross_domain_grounded);
        bool boundary_only_return_rejected = false;
        try {
            (void)make_grounded_phrase_return_hypothesis(
                canonical_graph,
                boundary_only,
                canonical_relation);
        } catch (const std::invalid_argument&) {
            boundary_only_return_rejected = true;
        }
        assert(boundary_only_return_rejected);

        // A performed event before the formal boundary is not re-onset.
        const node_id premature = add_performance_event(
            canonical_graph, "canonical premature onset", 395);
        bool premature_reonset_rejected = false;
        try {
            (void)make_performed_phrase_reonset_evidence(
                canonical_graph,
                at(400),
                {gesture(premature, melody, 395, 0.90)},
                16,
                phrase_b.span,
                phrase_role_formal_scale::local_phrase);
        } catch (const std::invalid_argument&) {
            premature_reonset_rejected = true;
        }
        assert(premature_reonset_rejected);
    }

    return 0;
}
