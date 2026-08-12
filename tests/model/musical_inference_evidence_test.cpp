#include "model/musical_execution_graph.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

using namespace vgmtooling::model;

namespace {

node_id add_performance_event(
    musical_execution_graph& graph,
    const char* label,
    std::int64_t onset,
    std::int64_t end,
    std::int64_t pitch_cents) {
    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = label;
    event.active = time_span{
        {time_domain::driver, onset, 1000, 0},
        time_coordinate{time_domain::driver, end, 1000, 0},
    };
    event.attributes.push_back({
        "normalized_pitch_cents",
        pitch_cents,
        evidence_status::derived,
        1.0,
        "cents",
    });
    event.provenance.push_back({
        evidence_status::derived,
        1.0,
        "layer-evidence-fixture",
        std::nullopt,
        "performance event deterministically recovered from lower execution evidence",
    });
    return graph.add_node(std::move(event));
}

node_id add_theory_hypothesis(
    musical_execution_graph& graph,
    const char* label,
    const char* theory,
    const char* scope,
    double confidence) {
    node analysis;
    analysis.kind = node_kind::musical_relation;
    analysis.layer = semantic_layer::musical_structure;
    analysis.flow = flow_kind::value;
    analysis.label = label;
    analysis.attributes.push_back({
        "analysis_kind",
        std::string{"harmonic_function"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    analysis.attributes.push_back({
        "analysis_model",
        std::string{theory},
        evidence_status::exact,
        1.0,
        "",
    });
    analysis.attributes.push_back({
        "model_scope",
        std::string{scope},
        evidence_status::exact,
        1.0,
        "",
    });
    analysis.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        theory,
        std::nullopt,
        "theory-level interpretation of unchanged performance evidence",
    });
    return graph.add_node(std::move(analysis));
}

node_id add_auditory_stream_hypothesis(
    musical_execution_graph& graph,
    const char* label,
    const char* model,
    const char* listener_scope,
    double confidence) {
    node stream;
    stream.kind = node_kind::auditory_stream;
    stream.layer = semantic_layer::auditory_interpretation;
    stream.flow = flow_kind::stream;
    stream.label = label;
    stream.attributes.push_back({
        "analysis_kind",
        std::string{"auditory_stream_grouping"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    stream.attributes.push_back({
        "analysis_model",
        std::string{model},
        evidence_status::exact,
        1.0,
        "",
    });
    stream.attributes.push_back({
        "listener_scope",
        std::string{listener_scope},
        evidence_status::exact,
        1.0,
        "",
    });
    stream.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        model,
        std::nullopt,
        "listener/model-level grouping hypothesis; not persistent musical-part truth",
    });
    return graph.add_node(std::move(stream));
}

void support_analysis(
    musical_execution_graph& graph,
    node_id source,
    node_id analysis,
    double confidence,
    const char* detail) {
    edge support;
    support.kind = edge_kind::derived_from;
    support.from = source;
    support.to = analysis;
    support.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        "layer-evidence-analysis",
        std::nullopt,
        detail,
    });
    graph.add_edge(std::move(support));
}

} // namespace

int main() {
    musical_execution_graph graph;

    // Exact source truth remains the floor beneath every later interpretation.
    node source;
    source.kind = node_kind::source_object;
    source.layer = semantic_layer::source_representation;
    source.flow = flow_kind::stream;
    source.label = "validated executable music source";
    source.attributes.push_back({
        "source_hash",
        std::string{"fixture-sha256"},
        evidence_status::exact,
        1.0,
        "",
    });
    source.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture-source",
        0u,
        "exact source identity",
    });
    const node_id source_id = graph.add_node(std::move(source));

    const node_id first_event = add_performance_event(graph, "event C", 0, 500, 6000);
    const node_id second_event = add_performance_event(graph, "event G", 0, 500, 6700);

    edge first_origin;
    first_origin.kind = edge_kind::derived_from;
    first_origin.from = source_id;
    first_origin.to = first_event;
    first_origin.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-source",
        std::nullopt,
        "performance event recovered from source execution",
    });
    graph.add_edge(std::move(first_origin));

    edge second_origin;
    second_origin.kind = edge_kind::derived_from;
    second_origin.from = source_id;
    second_origin.to = second_event;
    second_origin.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-source",
        std::nullopt,
        "performance event recovered from source execution",
    });
    graph.add_edge(std::move(second_origin));

    // The same exact/derived pitch evidence may admit more than one theoretical
    // interpretation. Neither analysis becomes a property of the source bytes.
    const node_id tonal_analysis = add_theory_hypothesis(
        graph,
        "dominant-function interpretation",
        "western_tonal_function_fixture",
        "western_common_practice",
        0.72);
    const node_id modal_analysis = add_theory_hypothesis(
        graph,
        "modal-center interpretation",
        "modal_analysis_fixture",
        "mode_relative",
        0.61);

    support_analysis(
        graph,
        first_event,
        tonal_analysis,
        0.72,
        "event supports one tonal-functional interpretation");
    support_analysis(
        graph,
        second_event,
        tonal_analysis,
        0.72,
        "event supports one tonal-functional interpretation");
    support_analysis(
        graph,
        first_event,
        modal_analysis,
        0.61,
        "same event supports an alternate modal interpretation");
    support_analysis(
        graph,
        second_event,
        modal_analysis,
        0.61,
        "same event supports an alternate modal interpretation");

    // Acoustic contributions are projections of the performance events. They
    // provide the proper substrate for listener-level grouping hypotheses.
    node first_acoustic;
    first_acoustic.kind = node_kind::acoustic_contribution;
    first_acoustic.layer = semantic_layer::acoustic_realization;
    first_acoustic.flow = flow_kind::stream;
    first_acoustic.label = "acoustic contribution C";
    const node_id first_acoustic_id = graph.add_node(std::move(first_acoustic));

    node second_acoustic;
    second_acoustic.kind = node_kind::acoustic_contribution;
    second_acoustic.layer = semantic_layer::acoustic_realization;
    second_acoustic.flow = flow_kind::stream;
    second_acoustic.label = "acoustic contribution G";
    const node_id second_acoustic_id = graph.add_node(std::move(second_acoustic));

    edge first_render;
    first_render.kind = edge_kind::contributes_to;
    first_render.from = first_event;
    first_render.to = first_acoustic_id;
    first_render.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-render",
        std::nullopt,
        "performance event contributes to rendered sound",
    });
    graph.add_edge(std::move(first_render));

    edge second_render;
    second_render.kind = edge_kind::contributes_to;
    second_render.from = second_event;
    second_render.to = second_acoustic_id;
    second_render.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-render",
        std::nullopt,
        "performance event contributes to rendered sound",
    });
    graph.add_edge(std::move(second_render));

    const node_id fused_stream = add_auditory_stream_hypothesis(
        graph,
        "fused harmonic stream",
        "harmonicity_onset_grouping_fixture",
        "generic_listener_model",
        0.68);
    const node_id segregated_stream = add_auditory_stream_hypothesis(
        graph,
        "segregated upper stream",
        "pitch_timbre_segregation_fixture",
        "generic_listener_model",
        0.57);

    edge fused_first;
    fused_first.kind = edge_kind::groups_into;
    fused_first.from = first_acoustic_id;
    fused_first.to = fused_stream;
    fused_first.provenance.push_back({
        evidence_status::hypothesis,
        0.68,
        "harmonicity_onset_grouping_fixture",
        std::nullopt,
        "common onset/harmonic relation supports perceptual fusion",
    });
    graph.add_edge(std::move(fused_first));

    edge fused_second;
    fused_second.kind = edge_kind::groups_into;
    fused_second.from = second_acoustic_id;
    fused_second.to = fused_stream;
    fused_second.provenance.push_back({
        evidence_status::hypothesis,
        0.68,
        "harmonicity_onset_grouping_fixture",
        std::nullopt,
        "common onset/harmonic relation supports perceptual fusion",
    });
    graph.add_edge(std::move(fused_second));

    edge segregated_second;
    segregated_second.kind = edge_kind::groups_into;
    segregated_second.from = second_acoustic_id;
    segregated_second.to = segregated_stream;
    segregated_second.provenance.push_back({
        evidence_status::hypothesis,
        0.57,
        "pitch_timbre_segregation_fixture",
        std::nullopt,
        "alternate listener model segregates this contribution",
    });
    graph.add_edge(std::move(segregated_second));

    // Historical attribution is external evidence. The archival statement can
    // be observed exactly while the musical attribution it reports remains a
    // hypothesis. This must not mutate executable source truth.
    node attribution;
    attribution.kind = node_kind::source_object;
    attribution.layer = semantic_layer::source_representation;
    attribution.flow = flow_kind::value;
    attribution.label = "external attribution annotation";
    attribution.attributes.push_back({
        "claim_kind",
        std::string{"composer_attribution"},
        evidence_status::exact,
        1.0,
        "",
    });
    attribution.attributes.push_back({
        "claim_value",
        std::string{"composer-X"},
        evidence_status::hypothesis,
        0.55,
        "",
    });
    attribution.provenance.push_back({
        evidence_status::exact,
        1.0,
        "external-catalog-fixture",
        std::nullopt,
        "the external source is quoted exactly; its attribution is not execution truth",
        to_flags(provenance_flag::external_annotation),
    });
    const node_id attribution_id = graph.add_node(std::move(attribution));

    edge annotation_target;
    annotation_target.kind = edge_kind::references;
    annotation_target.from = attribution_id;
    annotation_target.to = source_id;
    annotation_target.provenance.push_back({
        evidence_status::exact,
        1.0,
        "external-catalog-fixture",
        std::nullopt,
        "external annotation refers to this executable source",
        to_flags(provenance_flag::external_annotation),
    });
    graph.add_edge(std::move(annotation_target));

    // Competing theory analyses coexist over the same lower events.
    const auto first_theory_support = graph.edges_from(first_event, edge_kind::derived_from);
    assert(first_theory_support.size() == 2);
    assert(first_theory_support[0]->to != first_theory_support[1]->to);
    assert(graph.find_node(tonal_analysis)->layer == semantic_layer::musical_structure);
    assert(graph.find_node(modal_analysis)->layer == semantic_layer::musical_structure);
    assert(graph.find_node(tonal_analysis)->provenance[0].status == evidence_status::hypothesis);
    assert(graph.find_node(modal_analysis)->provenance[0].status == evidence_status::hypothesis);

    // Competing perceptual groupings can coexist without becoming part identity.
    const auto second_groupings = graph.edges_from(second_acoustic_id, edge_kind::groups_into);
    assert(second_groupings.size() == 2);
    assert(second_groupings[0]->to != second_groupings[1]->to);
    assert(graph.find_node(fused_stream)->kind == node_kind::auditory_stream);
    assert(graph.find_node(segregated_stream)->kind == node_kind::auditory_stream);
    assert(graph.find_node(fused_stream)->layer == semantic_layer::auditory_interpretation);
    assert(graph.nodes_of_kind(node_kind::part).empty());

    // The historical source and its reported claim retain separate evidence grades.
    const node* attribution_node = graph.find_node(attribution_id);
    assert(attribution_node != nullptr);
    assert(has_flag(attribution_node->provenance[0].flags, provenance_flag::external_annotation));
    assert(attribution_node->attributes[0].status == evidence_status::exact);
    assert(attribution_node->attributes[1].status == evidence_status::hypothesis);

    // None of the analyses has altered the lower source/performance evidence.
    const node* source_node = graph.find_node(source_id);
    const node* first_event_node = graph.find_node(first_event);
    const node* second_event_node = graph.find_node(second_event);
    assert(source_node != nullptr);
    assert(first_event_node != nullptr);
    assert(second_event_node != nullptr);
    assert(source_node->provenance[0].status == evidence_status::exact);
    assert(first_event_node->provenance[0].status == evidence_status::derived);
    assert(second_event_node->provenance[0].status == evidence_status::derived);
    assert(first_event_node->attributes[0].value == attribute_value{std::int64_t{6000}});
    assert(second_event_node->attributes[0].value == attribute_value{std::int64_t{6700}});

    return 0;
}
