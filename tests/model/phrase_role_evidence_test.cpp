#include "model/phrase_role_evidence.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_span span(std::int64_t start, std::int64_t end) {
    return {
        {time_domain::source, start, 44100, 0},
        time_coordinate{time_domain::source, end, 44100, 0},
    };
}

node_id add_support_node(musical_execution_graph& graph, const char* label) {
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
    node_id support_node,
    phrase_role_evidence_polarity polarity = phrase_role_evidence_polarity::supports) {
    phrase_role_evidence result;
    result.role = role;
    result.scope = scope;
    result.formal_scale = scale;
    result.origin = origin;
    result.polarity = polarity;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "phrase-role regression evidence";
    result.support_nodes = {support_node};
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id motif = add_support_node(graph, "motif completion");
    const node_id boundary = add_support_node(graph, "phrase boundary");
    const node_id parts = add_support_node(graph, "persistent parts continue");
    const node_id harmony = add_support_node(graph, "harmonic process continues");

    const auto local_scope = span(100, 200);

    // Ending and continuation remain separate candidates at the same formal
    // scale. The object preserves the explicit incompatibility rather than
    // choosing a cadence label or voting one candidate away.
    auto ending = make_phrase_role_hypothesis(
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
                "motif-analysis",
                motif),
            evidence(
                phrase_role_kind::ending,
                local_scope,
                phrase_role_formal_scale::local_phrase,
                phrase_role_evidence_origin::phrase_boundary_analysis,
                0.84,
                "boundary-analysis",
                boundary),
        },
        {phrase_role_kind::continuation});
    assert(ending.cross_domain_grounded);
    assert(close_enough(ending.confidence, 0.84));
    assert(!ending.role_established);

    auto continuation = make_phrase_role_hypothesis(
        phrase_role_kind::continuation,
        local_scope,
        phrase_role_formal_scale::local_phrase,
        0.91,
        {
            evidence(
                phrase_role_kind::continuation,
                local_scope,
                phrase_role_formal_scale::local_phrase,
                phrase_role_evidence_origin::persistent_part_continuation,
                0.89,
                "part-continuation",
                parts),
            evidence(
                phrase_role_kind::continuation,
                local_scope,
                phrase_role_formal_scale::local_phrase,
                phrase_role_evidence_origin::harmonic_process,
                0.83,
                "harmonic-process",
                harmony),
        },
        {phrase_role_kind::ending});
    assert(continuation.cross_domain_grounded);
    assert(close_enough(continuation.confidence, 0.83));

    const auto conflict = make_phrase_role_evidence_set({ending, continuation});
    assert(conflict.alternatives.size() == 2);
    assert(conflict.incompatible_alternatives_preserved);
    assert(!conflict.cross_scale_coexistence_preserved);

    // A local ending can coexist with larger-scale continuation without being
    // misreported as a same-scale contradiction.
    const auto global_scope = span(100, 400);
    auto global_continuation = make_phrase_role_hypothesis(
        phrase_role_kind::continuation,
        global_scope,
        phrase_role_formal_scale::phrase_group,
        0.88,
        {
            evidence(
                phrase_role_kind::continuation,
                global_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::persistent_part_continuation,
                0.85,
                "larger-scale-parts",
                parts),
            evidence(
                phrase_role_kind::continuation,
                global_scope,
                phrase_role_formal_scale::phrase_group,
                phrase_role_evidence_origin::harmonic_process,
                0.82,
                "larger-scale-harmony",
                harmony),
        });
    const auto nested = make_phrase_role_evidence_set({ending, global_continuation});
    assert(nested.cross_scale_coexistence_preserved);
    assert(!nested.incompatible_alternatives_preserved);

    const node_id ending_node = add_phrase_role_hypothesis(graph, ending);
    const node* materialized = graph.find_node(ending_node);
    assert(materialized != nullptr);
    assert(materialized->kind == node_kind::musical_relation);
    assert(materialized->layer == semantic_layer::musical_structure);
    assert(materialized->active.has_value());
    assert(materialized->active->start.tick == 100);
    assert(materialized->active->end.has_value());
    assert(materialized->active->end->tick == 200);
    assert(graph.edges_to(ending_node, edge_kind::derived_from).size() == 2);

    // One-sided declarations are malformed. Incompatibility is explicit state,
    // not an inference the container silently invents.
    bool asymmetric_rejected = false;
    try {
        auto malformed = continuation;
        malformed.incompatible_alternatives.clear();
        (void)make_phrase_role_evidence_set({ending, malformed});
    } catch (const std::invalid_argument&) {
        asymmetric_rejected = true;
    }
    assert(asymmetric_rejected);

    // Evidence for another role cannot be smuggled into a candidate merely to
    // raise its support ceiling.
    bool wrong_role_rejected = false;
    try {
        (void)make_phrase_role_hypothesis(
            phrase_role_kind::ending,
            local_scope,
            phrase_role_formal_scale::local_phrase,
            0.90,
            {
                evidence(
                    phrase_role_kind::continuation,
                    local_scope,
                    phrase_role_formal_scale::local_phrase,
                    phrase_role_evidence_origin::persistent_part_continuation,
                    0.90,
                    "wrong-role",
                    parts),
            });
    } catch (const std::invalid_argument&) {
        wrong_role_rejected = true;
    }
    assert(wrong_role_rejected);

    return 0;
}
