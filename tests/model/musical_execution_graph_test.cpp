#include "model/musical_execution_graph.h"

#include <cassert>
#include <stdexcept>

using namespace vgmtooling::model;

int main() {
    musical_execution_graph graph;

    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::event;
    part.label = "persistent musical part";
    part.provenance.push_back({evidence_status::derived, 0.95, "fixture", 0x120u, "recovered logical identity"});
    const auto part_id = graph.add_node(part);

    node instrument;
    instrument.kind = node_kind::instrument_definition;
    instrument.layer = semantic_layer::synthesis;
    instrument.flow = flow_kind::value;
    instrument.label = "FM patch";
    instrument.attributes.push_back({"algorithm", std::uint64_t{5}, evidence_status::exact, 1.0, ""});
    const auto instrument_id = graph.add_node(instrument);

    node first_voice;
    first_voice.kind = node_kind::voice_instance;
    first_voice.layer = semantic_layer::synthesis;
    first_voice.flow = flow_kind::stream;
    first_voice.label = "voice instance A";
    first_voice.active = time_span{
        {time_domain::device, 120, 44100, 0},
        time_coordinate{time_domain::device, 480, 44100, 0},
    };
    const auto first_voice_id = graph.add_node(first_voice);

    node second_voice = first_voice;
    second_voice.label = "voice instance B";
    second_voice.active = time_span{
        {time_domain::device, 600, 44100, 0},
        time_coordinate{time_domain::device, 900, 44100, 0},
    };
    const auto second_voice_id = graph.add_node(second_voice);

    node slot_two;
    slot_two.kind = node_kind::physical_slot;
    slot_two.layer = semantic_layer::driver_execution;
    slot_two.flow = flow_kind::value;
    slot_two.label = "YM2612 channel 2";
    slot_two.attributes.push_back({"channel", std::uint64_t{2}, evidence_status::exact, 1.0, ""});
    const auto slot_two_id = graph.add_node(slot_two);

    node slot_five = slot_two;
    slot_five.label = "YM2612 channel 5";
    slot_five.attributes[0].value = std::uint64_t{5};
    const auto slot_five_id = graph.add_node(slot_five);

    graph.add_edge({0, edge_kind::realizes, part_id, first_voice_id});
    graph.add_edge({0, edge_kind::realizes, part_id, second_voice_id});
    graph.add_edge({0, edge_kind::instantiates, instrument_id, first_voice_id});
    graph.add_edge({0, edge_kind::instantiates, instrument_id, second_voice_id});
    graph.add_edge({0, edge_kind::occupies, first_voice_id, slot_two_id});
    graph.add_edge({0, edge_kind::occupies, second_voice_id, slot_five_id});

    // A persistent musical identity can survive a physical-channel move.
    const auto realizations = graph.edges_from(part_id, edge_kind::realizes);
    assert(realizations.size() == 2);
    assert(realizations[0]->to != realizations[1]->to);

    const auto first_occupancy = graph.edges_from(first_voice_id, edge_kind::occupies);
    const auto second_occupancy = graph.edges_from(second_voice_id, edge_kind::occupies);
    assert(first_occupancy.size() == 1);
    assert(second_occupancy.size() == 1);
    assert(first_occupancy[0]->to == slot_two_id);
    assert(second_occupancy[0]->to == slot_five_id);

    // Definition, instance, physical slot and musical part remain distinct.
    assert(graph.find_node(instrument_id)->kind == node_kind::instrument_definition);
    assert(graph.find_node(first_voice_id)->kind == node_kind::voice_instance);
    assert(graph.find_node(slot_two_id)->kind == node_kind::physical_slot);
    assert(graph.find_node(part_id)->kind == node_kind::part);

    // Flow semantics are not flattened into one event type.
    assert(graph.find_node(part_id)->flow == flow_kind::event);
    assert(graph.find_node(instrument_id)->flow == flow_kind::value);
    assert(graph.find_node(first_voice_id)->flow == flow_kind::stream);

    // Exact source evidence and derived musical interpretation can coexist.
    assert(graph.find_node(slot_two_id)->attributes[0].status == evidence_status::exact);
    assert(graph.find_node(part_id)->provenance[0].status == evidence_status::derived);

    // Time coordinates retain their declared clock domain and loop identity.
    const auto& span = *graph.find_node(first_voice_id)->active;
    assert(span.start.domain == time_domain::device);
    assert(span.start.tick_rate == 44100);
    assert(span.start.loop_iteration == 0);
    assert(span.end.has_value());

    bool rejected_unknown_node = false;
    try {
        graph.add_edge({0, edge_kind::causes, part_id, 999999});
    } catch (const std::invalid_argument&) {
        rejected_unknown_node = true;
    }
    assert(rejected_unknown_node);

    return 0;
}
