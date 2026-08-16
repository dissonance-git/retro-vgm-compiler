#include "model/ludic_function_hypothesis.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_fixture_node(
    musical_execution_graph& graph,
    semantic_layer layer,
    node_kind kind,
    const char* label) {
    node value;
    value.kind = kind;
    value.layer = layer;
    value.flow = flow_kind::value;
    value.label = label;
    return graph.add_node(std::move(value));
}

const attribute* find_attribute(const node& value, const char* key) {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

const attribute* find_attribute(const edge& value, const char* key) {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

bool close_enough(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;

    const node_id loop_structure = add_fixture_node(
        graph,
        semantic_layer::musical_structure,
        node_kind::musical_relation,
        "stable repeating loop with restrained cadence");

    // Strong intrinsic evidence may suggest a ludic function, but audio/music
    // structure by itself cannot establish what the game is asking the music
    // to do. The policy therefore caps the contextual claim.
    auto place_hypothesis = make_ludic_function_hypothesis(
        ludic_function_kind::place_identity,
        0.93,
        {
            {
                ludic_evidence_origin::musical_intrinsic,
                ludic_evidence_polarity::supports,
                evidence_status::derived,
                0.93,
                "fixture-structure-analysis",
                "persistent loop identity and low transition pressure are consistent with a location-stabilizing cue",
                {loop_structure},
            },
        });

    assert(!place_hypothesis.context_grounded);
    assert(close_enough(place_hypothesis.proposed_confidence, 0.93));
    assert(close_enough(place_hypothesis.confidence, ludic_intrinsic_only_confidence_ceiling));
    assert(place_hypothesis.status == evidence_status::hypothesis);

    const node_id place_relation = add_ludic_function_hypothesis(graph, place_hypothesis);
    const node* place_node = graph.find_node(place_relation);
    assert(place_node != nullptr);
    assert(place_node->kind == node_kind::musical_relation);
    assert(place_node->layer == semantic_layer::musicological_context);

    const attribute* place_function = find_attribute(*place_node, "ludic_function");
    const attribute* place_grounding = find_attribute(*place_node, "context_grounded");
    assert(place_function != nullptr);
    assert(place_grounding != nullptr);
    assert(std::get<std::string>(place_function->value) == "place_identity");
    assert(!std::get<bool>(place_grounding->value));
    assert(close_enough(place_function->confidence, ludic_intrinsic_only_confidence_ceiling));

    const auto place_support_edges = graph.edges_to(place_relation, edge_kind::derived_from);
    assert(place_support_edges.size() == 1);
    assert(place_support_edges[0]->from == loop_structure);
    assert(std::get<std::string>(find_attribute(*place_support_edges[0], "evidence_polarity")->value) ==
           "supports");
    assert(std::get<std::string>(find_attribute(*place_support_edges[0], "evidence_origin")->value) ==
           "musical_intrinsic");

    const node_id runtime_state = add_fixture_node(
        graph,
        semantic_layer::driver_execution,
        node_kind::execution_trace,
        "instrumented game-state transition");
    const node_id transition_shape = add_fixture_node(
        graph,
        semantic_layer::musical_structure,
        node_kind::musical_relation,
        "cue boundary aligned to transition");

    // Runtime/game-context evidence changes the epistemic situation. The same
    // high proposed confidence can now survive the context guardrail because
    // the game-state relation is directly observed rather than guessed from
    // musical form alone.
    auto transition_hypothesis = make_ludic_function_hypothesis(
        ludic_function_kind::transition_management,
        0.91,
        {
            {
                ludic_evidence_origin::musical_intrinsic,
                ludic_evidence_polarity::supports,
                evidence_status::derived,
                0.88,
                "fixture-boundary-analysis",
                "musical boundary has a strong discontinuity followed by a new stable texture",
                {transition_shape},
            },
            {
                ludic_evidence_origin::runtime_game_context,
                ludic_evidence_polarity::supports,
                evidence_status::exact,
                1.0,
                "fixture-runtime-capture",
                "the cue boundary is observed at the instrumented game-state transition",
                {runtime_state},
            },
        });

    assert(transition_hypothesis.context_grounded);
    assert(close_enough(transition_hypothesis.confidence, 0.91));

    const node_id transition_relation =
        add_ludic_function_hypothesis(graph, transition_hypothesis);
    const node* transition_node = graph.find_node(transition_relation);
    assert(transition_node != nullptr);
    assert(transition_node->layer == semantic_layer::musicological_context);
    assert(transition_node->provenance.size() == 2);
    assert(has_flag(transition_node->provenance[1].flags, provenance_flag::runtime_capture));

    const auto transition_edges = graph.edges_to(transition_relation, edge_kind::derived_from);
    assert(transition_edges.size() == 2);

    // Counterevidence is preserved independently rather than silently baked
    // into a single opaque confidence number. The inference system that
    // proposes confidence remains responsible for weighing it.
    auto action_hypothesis = make_ludic_function_hypothesis(
        ludic_function_kind::action_coupling,
        0.72,
        {
            {
                ludic_evidence_origin::runtime_game_context,
                ludic_evidence_polarity::supports,
                evidence_status::derived,
                0.81,
                "fixture-action-log",
                "musical accent repeatedly follows a player action within the measured response window",
                {runtime_state},
            },
            {
                ludic_evidence_origin::runtime_game_context,
                ludic_evidence_polarity::counters,
                evidence_status::derived,
                0.66,
                "fixture-action-log",
                "the same accent also occurs without the player action in part of the capture",
                {runtime_state},
            },
        });

    const node_id action_relation = add_ludic_function_hypothesis(graph, action_hypothesis);
    const auto action_edges = graph.edges_to(action_relation, edge_kind::derived_from);
    assert(action_edges.size() == 2);
    assert(std::get<std::string>(find_attribute(*action_edges[0], "evidence_polarity")->value) ==
           "supports");
    assert(std::get<std::string>(find_attribute(*action_edges[1], "evidence_polarity")->value) ==
           "counters");

    // Exact external documentation can ground context while the resulting
    // synthesized claim remains explicitly a hypothesis.
    auto documented_hypothesis = make_ludic_function_hypothesis(
        ludic_function_kind::character_identity,
        0.97,
        {
            {
                ludic_evidence_origin::external_annotation,
                ludic_evidence_polarity::supports,
                evidence_status::exact,
                1.0,
                "fixture-official-cue-sheet",
                "official cue sheet associates the theme with the named character",
                {loop_structure},
            },
        });
    assert(documented_hypothesis.context_grounded);
    assert(documented_hypothesis.status == evidence_status::hypothesis);
    assert(close_enough(documented_hypothesis.confidence, 0.97));

    const node_id documented_relation =
        add_ludic_function_hypothesis(graph, documented_hypothesis);
    const node* documented_node = graph.find_node(documented_relation);
    assert(documented_node != nullptr);
    assert(has_flag(documented_node->provenance[0].flags, provenance_flag::external_annotation));

    bool rejected_bad_confidence = false;
    try {
        (void)make_ludic_function_hypothesis(
            ludic_function_kind::narrative_frame,
            1.2,
            {{
                ludic_evidence_origin::musical_intrinsic,
                ludic_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.5,
                "fixture",
                "invalid outer confidence",
                {loop_structure},
            }});
    } catch (const std::invalid_argument&) {
        rejected_bad_confidence = true;
    }
    assert(rejected_bad_confidence);

    bool rejected_no_support = false;
    try {
        (void)make_ludic_function_hypothesis(
            ludic_function_kind::reward_feedback,
            0.4,
            {{
                ludic_evidence_origin::runtime_game_context,
                ludic_evidence_polarity::counters,
                evidence_status::derived,
                0.7,
                "fixture",
                "counterevidence without any positive witness",
                {runtime_state},
            }});
    } catch (const std::invalid_argument&) {
        rejected_no_support = true;
    }
    assert(rejected_no_support);

    return 0;
}
