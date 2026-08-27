#include "model/phrase_role_scale_arbitration.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_span span(std::int64_t start, std::int64_t end) {
    return {
        {time_domain::source, start, 1000, 0},
        time_coordinate{time_domain::source, end, 1000, 0},
    };
}

node_id add_support(
    musical_execution_graph& graph,
    const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_structure;
    value.flow = flow_kind::event;
    value.label = label;
    return graph.add_node(std::move(value));
}

phrase_role_evidence evidence(
    phrase_role_kind role,
    time_span scope,
    phrase_role_formal_scale scale,
    phrase_role_evidence_origin origin,
    double confidence,
    const char* source,
    node_id support) {
    phrase_role_evidence result;
    result.role = role;
    result.scope = scope;
    result.formal_scale = scale;
    result.origin = origin;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "multi-scale arbitration regression evidence";
    result.support_nodes = {support};
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id motif = add_support(graph, "local motif completion");
    const node_id boundary = add_support(graph, "local phrase boundary");
    const node_id parts = add_support(graph, "larger persistent-part continuation");
    const node_id harmony = add_support(graph, "larger harmonic continuation");
    const node_id dependency = add_support(graph, "long-range harmonic dependency");

    const auto local_scope = span(120, 200);
    const auto outer_scope = span(100, 400);

    const auto local_ending = make_phrase_role_hypothesis(
        phrase_role_kind::ending,
        local_scope,
        phrase_role_formal_scale::local_phrase,
        0.90,
        {
            evidence(
                phrase_role_kind::ending,
                local_scope,
                phrase_role_formal_scale::local_phrase,
                phrase_role_evidence_origin::motif_analysis,
                0.86,
                "local-motif",
                motif),
            evidence(
                phrase_role_kind::ending,
                local_scope,
                phrase_role_formal_scale::local_phrase,
                phrase_role_evidence_origin::phrase_boundary_analysis,
                0.84,
                "local-boundary",
                boundary),
        });
    CHECK(local_ending.cross_domain_grounded);
    CHECK(close_enough(local_ending.confidence, 0.84));

    const auto global_continuation = make_phrase_role_hypothesis(
        phrase_role_kind::continuation,
        outer_scope,
        phrase_role_formal_scale::phrase_group,
        0.90,
        {
            evidence(
                phrase_role_kind::continuation,
                outer_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::persistent_part_continuation,
                0.85,
                "outer-parts",
                parts),
            evidence(
                phrase_role_kind::continuation,
                outer_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::harmonic_process,
                0.82,
                "outer-harmony",
                harmony),
        });
    CHECK(global_continuation.cross_domain_grounded);
    CHECK(close_enough(global_continuation.confidence, 0.82));

    const auto arbitration =
        infer_phrase_role_scale_arbitration(
            local_ending,
            global_continuation);
    CHECK(arbitration.kind == phrase_role_scale_relation_kind::
        local_close_inside_global_continuation);
    CHECK(arbitration.strict_nesting);
    CHECK(arbitration.cross_scale_coexistence_preserved);
    CHECK(arbitration.both_roles_cross_domain_grounded);
    CHECK(!arbitration.role_established);
    CHECK(close_enough(arbitration.confidence, 0.82));

    const auto composite =
        make_nested_local_close_inside_global_continuation_hypothesis(
            arbitration,
            local_ending,
            global_continuation);
    CHECK(composite.role ==
        phrase_role_kind::nested_local_close_inside_global_continuation);
    CHECK(same_phrase_role_scope(composite.scope, outer_scope));
    CHECK(composite.formal_scale ==
        phrase_role_formal_scale::phrase_group);
    CHECK(composite.cross_domain_grounded);
    CHECK(composite.support_domains == 4);
    CHECK(!composite.role_established);
    CHECK(close_enough(composite.confidence, 0.82));

    const node_id composite_node =
        add_phrase_role_hypothesis(graph, composite);
    CHECK(graph.find_node(composite_node) != nullptr);
    CHECK(graph.edges_to(
        composite_node,
        edge_kind::derived_from).size() == 4);

    // Larger-scale prolongation is preserved as a different nested relation.
    // It is not silently renamed to the continuation-specific composite role.
    const auto global_prolongation = make_phrase_role_hypothesis(
        phrase_role_kind::prolongation,
        outer_scope,
        phrase_role_formal_scale::phrase_group,
        0.88,
        {
            evidence(
                phrase_role_kind::prolongation,
                outer_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::harmonic_dependency,
                0.84,
                "harmonic-span",
                dependency),
            evidence(
                phrase_role_kind::prolongation,
                outer_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::persistent_part_continuation,
                0.81,
                "outer-part-continuity",
                parts),
        });
    const auto prolongation_arbitration =
        infer_phrase_role_scale_arbitration(
            local_ending,
            global_prolongation);
    CHECK(prolongation_arbitration.kind ==
        phrase_role_scale_relation_kind::
            local_close_inside_global_prolongation);
    CHECK(prolongation_arbitration.cross_scale_coexistence_preserved);

    bool wrong_composite_rejected = false;
    try {
        (void)make_nested_local_close_inside_global_continuation_hypothesis(
            prolongation_arbitration,
            local_ending,
            global_continuation);
    } catch (const std::invalid_argument&) {
        wrong_composite_rejected = true;
    }
    CHECK(wrong_composite_rejected);

    // Same-scale disagreement belongs to the ordinary alternative-preservation
    // machinery, not to cross-scale nesting.
    bool same_scale_rejected = false;
    try {
        auto same_scale = global_continuation;
        same_scale.formal_scale =
            phrase_role_formal_scale::local_phrase;
        (void)infer_phrase_role_scale_arbitration(
            local_ending,
            same_scale);
    } catch (const std::invalid_argument&) {
        same_scale_rejected = true;
    }
    CHECK(same_scale_rejected);

    // Temporal overlap is not enough. The local claim must be wholly contained
    // by the larger explicit scope.
    bool overlap_only_rejected = false;
    try {
        auto partial = global_continuation;
        partial.scope = span(100, 180);
        (void)infer_phrase_role_scale_arbitration(
            local_ending,
            partial);
    } catch (const std::invalid_argument&) {
        overlap_only_rejected = true;
    }
    CHECK(overlap_only_rejected);

    // A weak single-domain larger candidate may still be retained as a nested
    // alternative, but it cannot earn the canonical composite role.
    const auto weak_outer = make_phrase_role_hypothesis(
        phrase_role_kind::continuation,
        outer_scope,
        phrase_role_formal_scale::phrase_group,
        0.90,
        {
            evidence(
                phrase_role_kind::continuation,
                outer_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::harmonic_process,
                0.88,
                "single-domain",
                harmony),
        });
    const auto weak_arbitration =
        infer_phrase_role_scale_arbitration(
            local_ending,
            weak_outer);
    CHECK(weak_arbitration.cross_scale_coexistence_preserved);
    CHECK(!weak_arbitration.both_roles_cross_domain_grounded);

    bool weak_composite_rejected = false;
    try {
        (void)make_nested_local_close_inside_global_continuation_hypothesis(
            weak_arbitration,
            local_ending,
            weak_outer);
    } catch (const std::invalid_argument&) {
        weak_composite_rejected = true;
    }
    CHECK(weak_composite_rejected);

    return 0;
}
