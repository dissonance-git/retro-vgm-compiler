#include "model/musical_execution_graph.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;

namespace {

node_id add_performance_event(
    musical_execution_graph& graph,
    const char* label,
    std::int64_t onset_ms,
    std::int64_t end_ms,
    std::int64_t pitch_cents) {
    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = label;
    event.active = time_span{
        {time_domain::driver, onset_ms, 1000, 0},
        time_coordinate{time_domain::driver, end_ms, 1000, 0},
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
        "voice-separation-fixture",
        std::nullopt,
        "performance event deterministically recovered from lower execution evidence",
    });
    return graph.add_node(std::move(event));
}

node_id add_physical_episode(
    musical_execution_graph& graph,
    const char* label,
    std::uint64_t physical_channel) {
    node voice;
    voice.kind = node_kind::voice_instance;
    voice.layer = semantic_layer::synthesis;
    voice.flow = flow_kind::stream;
    voice.label = label;
    voice.attributes.push_back({
        "physical_channel",
        physical_channel,
        evidence_status::derived,
        1.0,
        "channel",
    });
    voice.attributes.push_back({
        "identity_scope",
        std::string{"physical_voice_episode"},
        evidence_status::derived,
        1.0,
        "",
    });
    return graph.add_node(std::move(voice));
}

node_id add_part_hypothesis(
    musical_execution_graph& graph,
    const char* label,
    double confidence,
    const char* detail) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = label;
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_voice_hypothesis"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    part.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        "voice-separation-analysis",
        std::nullopt,
        detail,
    });
    return graph.add_node(std::move(part));
}

void group_candidate(
    musical_execution_graph& graph,
    node_id event_id,
    node_id part_id,
    double confidence,
    std::int64_t pitch_distance_cents,
    std::int64_t time_gap_ms,
    bool same_physical_channel) {
    edge grouping;
    grouping.kind = edge_kind::groups_into;
    grouping.from = event_id;
    grouping.to = part_id;
    grouping.attributes.push_back({
        "analysis",
        std::string{"symbolic_voice_separation"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    grouping.attributes.push_back({
        "pitch_distance_cents",
        pitch_distance_cents,
        evidence_status::derived,
        1.0,
        "cents",
    });
    grouping.attributes.push_back({
        "time_gap_ms",
        time_gap_ms,
        evidence_status::derived,
        1.0,
        "ms",
    });
    grouping.attributes.push_back({
        "same_physical_channel",
        same_physical_channel,
        evidence_status::derived,
        1.0,
        "",
    });
    grouping.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        "voice-separation-analysis",
        std::nullopt,
        "candidate persistent-voice assignment; lower performance and synthesis evidence remains canonical",
    });
    graph.add_edge(std::move(grouping));
}

void group_from_driver_identity(
    musical_execution_graph& graph,
    node_id event_id,
    node_id part_id,
    const char* source,
    std::uint64_t offset) {
    edge grouping;
    grouping.kind = edge_kind::groups_into;
    grouping.from = event_id;
    grouping.to = part_id;
    grouping.attributes.push_back({
        "support_kind",
        std::string{"validated_driver_track_identity"},
        evidence_status::derived,
        1.0,
        "",
    });
    grouping.provenance.push_back({
        evidence_status::derived,
        1.0,
        source,
        offset,
        "validated driver grammar ties this event to a persistent logical track; source evidence outranks proximity-only grouping",
    });
    graph.add_edge(std::move(grouping));
}

} // namespace

int main() {
    musical_execution_graph graph;

    // A simple crossing fixture. The event labels intentionally do not encode
    // voice identity. Two physical channels trade register position, so pitch
    // height and hardware channel number cannot be used as identity by fiat.
    const node_id first = add_performance_event(graph, "event A", 0, 400, 6000);
    const node_id upper_after_crossing = add_performance_event(graph, "event B", 500, 900, 7200);
    const node_id lower_after_crossing = add_performance_event(graph, "event C", 500, 900, 5900);

    const node_id voice_ch0 = add_physical_episode(graph, "physical episode channel 0", 0);
    const node_id voice_ch3 = add_physical_episode(graph, "physical episode channel 3", 3);

    edge first_realization;
    first_realization.kind = edge_kind::realizes;
    first_realization.from = first;
    first_realization.to = voice_ch0;
    graph.add_edge(first_realization);

    edge crossed_realization;
    crossed_realization.kind = edge_kind::realizes;
    crossed_realization.from = upper_after_crossing;
    crossed_realization.to = voice_ch3;
    graph.add_edge(crossed_realization);

    // Competing hypotheses are legal. Candidate 1 follows a large upward leap
    // across a physical-channel change; candidate 2 follows local pitch
    // proximity. Neither interpretation overwrites the recovered events.
    const node_id crossing_part = add_part_hypothesis(
        graph,
        "candidate voice through register crossing",
        0.62,
        "candidate trajectory supported by non-overlap and contextual continuity despite register/channel change");
    const node_id proximity_part = add_part_hypothesis(
        graph,
        "candidate voice by local pitch proximity",
        0.74,
        "candidate trajectory supported by local pitch and temporal proximity");

    group_candidate(graph, first, crossing_part, 0.62, 1200, 100, false);
    group_candidate(graph, upper_after_crossing, crossing_part, 0.62, 1200, 100, false);

    group_candidate(graph, first, proximity_part, 0.74, 100, 100, false);
    group_candidate(graph, lower_after_crossing, proximity_part, 0.74, 100, 100, false);

    const auto first_groupings = graph.edges_from(first, edge_kind::groups_into);
    assert(first_groupings.size() == 2);
    assert(first_groupings[0]->to != first_groupings[1]->to);

    const node* crossing = graph.find_node(crossing_part);
    const node* proximity = graph.find_node(proximity_part);
    assert(crossing != nullptr);
    assert(proximity != nullptr);
    assert(crossing->kind == node_kind::part);
    assert(proximity->kind == node_kind::part);
    assert(crossing->provenance[0].status == evidence_status::hypothesis);
    assert(proximity->provenance[0].status == evidence_status::hypothesis);

    // The exact/derived lower evidence remains the same no matter how many
    // higher-level grouping hypotheses are added.
    assert(graph.find_node(first)->kind == node_kind::musical_event);
    assert(graph.find_node(upper_after_crossing)->kind == node_kind::musical_event);
    assert(graph.find_node(lower_after_crossing)->kind == node_kind::musical_event);
    assert(graph.find_node(voice_ch0)->kind == node_kind::voice_instance);
    assert(graph.find_node(voice_ch3)->kind == node_kind::voice_instance);

    // Physical allocation is evidence, not musical identity. The graph must not
    // require an identity edge between hardware episodes for a candidate part to
    // span them.
    assert(graph.edges_from(voice_ch0, edge_kind::same_identity_as).empty());
    assert(graph.edges_from(voice_ch3, edge_kind::same_identity_as).empty());

    // A candidate grouping edge itself remains explicitly hypothetical, while
    // its measured feature values can be derived from lower evidence.
    for (const edge* grouping : first_groupings) {
        assert(grouping->provenance.size() == 1);
        assert(grouping->provenance[0].status == evidence_status::hypothesis);
        assert(grouping->attributes.size() == 4);
        assert(grouping->attributes[1].status == evidence_status::derived);
        assert(grouping->attributes[2].status == evidence_status::derived);
        assert(grouping->attributes[3].status == evidence_status::derived);
    }

    // VGM Tooling can often know more than a score-only voice-separation system.
    // Here a validated driver track proves that event A and event B belong to the
    // same persistent logical source even though the proximity-only candidate
    // prefers event C. Stronger source evidence is represented as stronger
    // evidence, not by deleting the weaker hypothesis.
    node driver_track;
    driver_track.kind = node_kind::logical_process;
    driver_track.layer = semantic_layer::driver_execution;
    driver_track.flow = flow_kind::control;
    driver_track.label = "validated logical driver track";
    driver_track.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture-driver",
        0x200u,
        "validated track grammar and execution identity",
    });
    const node_id driver_track_id = graph.add_node(std::move(driver_track));

    node supported_part;
    supported_part.kind = node_kind::part;
    supported_part.layer = semantic_layer::musical_performance;
    supported_part.flow = flow_kind::stream;
    supported_part.label = "driver-supported persistent musical part";
    supported_part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::derived,
        1.0,
        "",
    });
    supported_part.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-driver",
        0x200u,
        "persistent part derived from validated logical driver-track identity",
    });
    const node_id supported_part_id = graph.add_node(std::move(supported_part));

    edge part_derivation;
    part_derivation.kind = edge_kind::derived_from;
    part_derivation.from = driver_track_id;
    part_derivation.to = supported_part_id;
    part_derivation.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-driver",
        0x200u,
        "driver identity supports the persistent musical part",
    });
    graph.add_edge(std::move(part_derivation));

    group_from_driver_identity(graph, first, supported_part_id, "fixture-driver", 0x210u);
    group_from_driver_identity(graph, upper_after_crossing, supported_part_id, "fixture-driver", 0x240u);

    const auto first_groupings_with_source = graph.edges_from(first, edge_kind::groups_into);
    assert(first_groupings_with_source.size() == 3);

    const node* source_supported = graph.find_node(supported_part_id);
    assert(source_supported != nullptr);
    assert(source_supported->provenance[0].status == evidence_status::derived);
    assert(source_supported->provenance[0].confidence == 1.0);

    const auto supported_first = graph.edges_to(supported_part_id, edge_kind::groups_into);
    assert(supported_first.size() == 2);
    for (const edge* grouping : supported_first) {
        assert(grouping->provenance[0].status == evidence_status::derived);
        assert(grouping->provenance[0].confidence == 1.0);
    }

    // The lower-confidence alternatives remain inspectable. Evidence hierarchy
    // resolves what should be trusted without erasing why another analysis found
    // a different candidate trajectory.
    assert(graph.find_node(crossing_part)->provenance[0].status == evidence_status::hypothesis);
    assert(graph.find_node(proximity_part)->provenance[0].status == evidence_status::hypothesis);

    return 0;
}
