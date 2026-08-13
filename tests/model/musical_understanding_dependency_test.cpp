#include "model/musical_execution_graph.h"

#include <cassert>
#include <optional>
#include <string>

using namespace vgmtooling::model;

namespace {

node_id add_analysis(
    musical_execution_graph& graph,
    const char* label,
    semantic_layer layer,
    const char* analysis_kind,
    const char* method,
    double confidence) {
    node value;
    value.kind = node_kind::musical_relation;
    value.layer = layer;
    value.flow = flow_kind::value;
    value.label = label;
    value.attributes.push_back({
        "analysis_kind",
        std::string{analysis_kind},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    value.attributes.push_back({
        "analysis_method",
        std::string{method},
        evidence_status::exact,
        1.0,
        "",
    });
    value.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        method,
        std::nullopt,
        "analysis remains dependent on its declared lower evidence",
    });
    return graph.add_node(std::move(value));
}

edge_id derive(
    musical_execution_graph& graph,
    node_id from,
    node_id to,
    evidence_status status,
    double confidence,
    const char* source,
    const char* detail) {
    edge value;
    value.kind = edge_kind::derived_from;
    value.from = from;
    value.to = to;
    value.provenance.push_back({
        status,
        confidence,
        source,
        std::nullopt,
        detail,
    });
    return graph.add_edge(std::move(value));
}

bool has_direct_dependency(
    const musical_execution_graph& graph,
    node_id dependency,
    node_id claim) {
    for (const edge* value : graph.edges_from(dependency, edge_kind::derived_from)) {
        if (value->to == claim)
            return true;
    }
    return false;
}

} // namespace

int main() {
    musical_execution_graph graph;

    // Exact executable evidence remains unchanged beneath every analysis.
    node source;
    source.kind = node_kind::source_object;
    source.layer = semantic_layer::source_representation;
    source.flow = flow_kind::stream;
    source.label = "exact executable music object";
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
        "immutable source evidence",
    });
    const node_id source_id = graph.add_node(std::move(source));

    // Two recovered performance spans stand in for the exact-to-derived event
    // route that real VGM/driver adapters must provide.
    node first_pitch;
    first_pitch.kind = node_kind::musical_event;
    first_pitch.layer = semantic_layer::musical_performance;
    first_pitch.flow = flow_kind::event;
    first_pitch.label = "performed pitch span A";
    first_pitch.attributes.push_back({
        "normalized_pitch_cents",
        std::int64_t{6000},
        evidence_status::derived,
        1.0,
        "cents",
    });
    first_pitch.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-performance-adapter",
        std::nullopt,
        "pitch recovered from executable state",
    });
    const node_id first_pitch_id = graph.add_node(std::move(first_pitch));

    node second_pitch;
    second_pitch.kind = node_kind::musical_event;
    second_pitch.layer = semantic_layer::musical_performance;
    second_pitch.flow = flow_kind::event;
    second_pitch.label = "performed pitch span B";
    second_pitch.attributes.push_back({
        "normalized_pitch_cents",
        std::int64_t{6400},
        evidence_status::derived,
        1.0,
        "cents",
    });
    second_pitch.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-performance-adapter",
        std::nullopt,
        "pitch recovered from executable state",
    });
    const node_id second_pitch_id = graph.add_node(std::move(second_pitch));

    derive(
        graph,
        source_id,
        first_pitch_id,
        evidence_status::derived,
        1.0,
        "fixture-performance-adapter",
        "source execution supports performed pitch A");
    derive(
        graph,
        source_id,
        second_pitch_id,
        evidence_status::derived,
        1.0,
        "fixture-performance-adapter",
        "source execution supports performed pitch B");

    // The acoustic/heard route is independent of persistent musical-part truth.
    node acoustic;
    acoustic.kind = node_kind::acoustic_contribution;
    acoustic.layer = semantic_layer::acoustic_realization;
    acoustic.flow = flow_kind::stream;
    acoustic.label = "rendered mixed contribution";
    const node_id acoustic_id = graph.add_node(std::move(acoustic));

    edge render;
    render.kind = edge_kind::contributes_to;
    render.from = first_pitch_id;
    render.to = acoustic_id;
    render.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-render",
        std::nullopt,
        "performance contributes to rendered evidence",
    });
    graph.add_edge(std::move(render));

    node heard_stream;
    heard_stream.kind = node_kind::auditory_stream;
    heard_stream.layer = semantic_layer::auditory_interpretation;
    heard_stream.flow = flow_kind::stream;
    heard_stream.label = "heard stream hypothesis";
    heard_stream.provenance.push_back({
        evidence_status::hypothesis,
        0.74,
        "libaural-shaped-grouping-fixture",
        std::nullopt,
        "auditory grouping evidence does not rewrite source or part identity",
    });
    const node_id heard_stream_id = graph.add_node(std::move(heard_stream));

    edge grouping;
    grouping.kind = edge_kind::groups_into;
    grouping.from = acoustic_id;
    grouping.to = heard_stream_id;
    grouping.provenance.push_back({
        evidence_status::hypothesis,
        0.74,
        "libaural-shaped-grouping-fixture",
        std::nullopt,
        "rendered evidence supports one heard-stream interpretation",
    });
    graph.add_edge(std::move(grouping));

    // Harmonic analysis is explicitly layered instead of mapping active pitches
    // straight to a final functional label.
    const node_id chord = add_analysis(
        graph,
        "chord candidate",
        semantic_layer::musical_structure,
        "chord_identity",
        "fixture_chord_analysis",
        0.82);
    derive(
        graph,
        first_pitch_id,
        chord,
        evidence_status::hypothesis,
        0.82,
        "fixture_chord_analysis",
        "performed pitch participates in the chord analysis");
    derive(
        graph,
        second_pitch_id,
        chord,
        evidence_status::hypothesis,
        0.82,
        "fixture_chord_analysis",
        "performed pitch participates in the chord analysis");

    const node_id local_key = add_analysis(
        graph,
        "local tonal-center candidate",
        semantic_layer::musical_structure,
        "local_tonal_center",
        "fixture_tonal_analysis",
        0.77);
    derive(
        graph,
        chord,
        local_key,
        evidence_status::hypothesis,
        0.77,
        "fixture_tonal_analysis",
        "chord interpretation contributes to local tonal-center analysis");

    const node_id progression = add_analysis(
        graph,
        "harmonic progression relation",
        semantic_layer::musical_structure,
        "harmonic_progression",
        "fixture_progression_analysis",
        0.73);
    derive(
        graph,
        chord,
        progression,
        evidence_status::hypothesis,
        0.73,
        "fixture_progression_analysis",
        "chord participates in progression analysis");
    derive(
        graph,
        local_key,
        progression,
        evidence_status::hypothesis,
        0.73,
        "fixture_progression_analysis",
        "local tonal context constrains progression function");

    const node_id cadence = add_analysis(
        graph,
        "cadential interpretation",
        semantic_layer::musical_structure,
        "cadence",
        "fixture_phrase_analysis",
        0.69);
    derive(
        graph,
        progression,
        cadence,
        evidence_status::hypothesis,
        0.69,
        "fixture_phrase_analysis",
        "progression supports one cadential interpretation");

    const node_id formal_return = add_analysis(
        graph,
        "transformed formal return",
        semantic_layer::musical_structure,
        "formal_relation",
        "fixture_form_analysis",
        0.66);
    derive(
        graph,
        cadence,
        formal_return,
        evidence_status::hypothesis,
        0.66,
        "fixture_form_analysis",
        "cadential placement contributes to the formal reading");

    // A second chord hypothesis may legitimately enter from an audio-inverse
    // route. It must coexist with, rather than overwrite, the source-authoritative
    // performance route.
    const node_id heard_chord = add_analysis(
        graph,
        "audio-inverse chord candidate",
        semantic_layer::musical_structure,
        "chord_identity",
        "fixture_auditory_harmony_analysis",
        0.58);
    derive(
        graph,
        heard_stream_id,
        heard_chord,
        evidence_status::hypothesis,
        0.58,
        "fixture_auditory_harmony_analysis",
        "heard grouping supports an independent chord interpretation");

    assert(chord != heard_chord);
    assert(has_direct_dependency(graph, first_pitch_id, chord));
    assert(has_direct_dependency(graph, heard_stream_id, heard_chord));

    // Technical realization evidence belongs to a different authorial coordinate.
    node technical_signature;
    technical_signature.kind = node_kind::musical_relation;
    technical_signature.layer = semantic_layer::driver_execution;
    technical_signature.flow = flow_kind::value;
    technical_signature.label = "implementation fingerprint";
    technical_signature.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-driver-analysis",
        std::nullopt,
        "technical realization evidence",
    });
    const node_id technical_signature_id = graph.add_node(std::move(technical_signature));

    const node_id arranger_candidate = add_analysis(
        graph,
        "arranger / implementation candidate",
        semantic_layer::musicological_context,
        "arrangement_implementation_attribution",
        "fixture_blind_realization_control",
        0.79);
    derive(
        graph,
        technical_signature_id,
        arranger_candidate,
        evidence_status::hypothesis,
        0.79,
        "fixture_blind_realization_control",
        "technical fingerprint supports the realization-role candidate only");

    // Independent documentary evidence is a valid side entrance at the upper
    // layer. It can constrain composer candidates without pretending to have
    // been derived from device behavior.
    node document;
    document.kind = node_kind::source_object;
    document.layer = semantic_layer::musicological_context;
    document.flow = flow_kind::value;
    document.label = "independent historical source";
    document.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture-archive",
        std::nullopt,
        "exactly observed historical witness",
        to_flags(provenance_flag::external_annotation),
    });
    const node_id document_id = graph.add_node(std::move(document));

    const node_id composer_candidate = add_analysis(
        graph,
        "composer candidate",
        semantic_layer::musicological_context,
        "composition_attribution",
        "fixture_musicological_candidate_test",
        0.64);
    derive(
        graph,
        formal_return,
        composer_candidate,
        evidence_status::hypothesis,
        0.64,
        "fixture_musicological_candidate_test",
        "composition-level formal evidence contributes to the candidate");
    derive(
        graph,
        document_id,
        composer_candidate,
        evidence_status::hypothesis,
        0.64,
        "fixture_musicological_candidate_test",
        "independent historical evidence constrains the candidate set");

    // The realization fingerprint is deliberately not a direct composer support.
    assert(!has_direct_dependency(graph, technical_signature_id, composer_candidate));
    assert(has_direct_dependency(graph, technical_signature_id, arranger_candidate));
    assert(has_direct_dependency(graph, formal_return, composer_candidate));
    assert(has_direct_dependency(graph, document_id, composer_candidate));

    // The harmonic route remains traversable one dependency at a time.
    assert(has_direct_dependency(graph, chord, local_key));
    assert(has_direct_dependency(graph, chord, progression));
    assert(has_direct_dependency(graph, local_key, progression));
    assert(has_direct_dependency(graph, progression, cadence));
    assert(has_direct_dependency(graph, cadence, formal_return));

    // Nothing above rewrites the exact source or derived performance evidence.
    const node* source_after = graph.find_node(source_id);
    const node* pitch_after = graph.find_node(first_pitch_id);
    assert(source_after != nullptr);
    assert(pitch_after != nullptr);
    assert(source_after->provenance[0].status == evidence_status::exact);
    assert(source_after->attributes[0].value == attribute_value{std::string{"fixture-sha256"}});
    assert(pitch_after->provenance[0].status == evidence_status::derived);
    assert(pitch_after->attributes[0].value == attribute_value{std::int64_t{6000}});

    // Auditory grouping remains a listener/model hypothesis rather than a part.
    assert(graph.find_node(heard_stream_id)->kind == node_kind::auditory_stream);
    assert(graph.nodes_of_kind(node_kind::part).empty());

    return 0;
}
