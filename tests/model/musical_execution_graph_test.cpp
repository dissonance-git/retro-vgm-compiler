#include "model/musical_execution_graph.h"

#include <cassert>
#include <stdexcept>

using namespace vgmtooling::model;

int main() {
    musical_execution_graph graph;

    const auto connect = [&graph](edge_kind kind, node_id from, node_id to) {
        edge relation;
        relation.kind = kind;
        relation.from = from;
        relation.to = to;
        return graph.add_edge(relation);
    };

    node captured_vgm;
    captured_vgm.kind = node_kind::source_object;
    captured_vgm.layer = semantic_layer::source_representation;
    captured_vgm.flow = flow_kind::stream;
    captured_vgm.label = "captured VGM trace";
    captured_vgm.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture.vgm",
        0u,
        "bytes are exact relative to this file, but capture is intentionally incomplete",
        provenance_flag::runtime_capture | provenance_flag::incomplete,
    });
    const auto captured_vgm_id = graph.add_node(captured_vgm);

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

    node harmonic_relation;
    harmonic_relation.kind = node_kind::musical_relation;
    harmonic_relation.layer = semantic_layer::musical_structure;
    harmonic_relation.flow = flow_kind::value;
    harmonic_relation.label = "harmonic relation hypothesis";
    harmonic_relation.attributes.push_back({"analysis", std::string{"dominant-function"}, evidence_status::hypothesis, 0.72, ""});
    const auto harmonic_relation_id = graph.add_node(harmonic_relation);

    node midi_projection;
    midi_projection.kind = node_kind::projection;
    midi_projection.layer = semantic_layer::musical_performance;
    midi_projection.flow = flow_kind::event;
    midi_projection.label = "MIDI diagnostic projection";
    midi_projection.provenance.push_back({evidence_status::derived, 1.0, "graph-export", std::nullopt, "lossy inspection view"});
    const auto midi_projection_id = graph.add_node(midi_projection);

    connect(edge_kind::derived_from, captured_vgm_id, part_id);
    connect(edge_kind::realizes, part_id, first_voice_id);
    connect(edge_kind::realizes, part_id, second_voice_id);
    connect(edge_kind::instantiates, instrument_id, first_voice_id);
    connect(edge_kind::instantiates, instrument_id, second_voice_id);
    connect(edge_kind::occupies, first_voice_id, slot_two_id);
    connect(edge_kind::occupies, second_voice_id, slot_five_id);
    connect(edge_kind::derived_from, part_id, harmonic_relation_id);
    connect(edge_kind::projects_to, part_id, midi_projection_id);

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

    // Definition, instance, physical slot, musical part and musical analysis remain distinct.
    assert(graph.find_node(instrument_id)->kind == node_kind::instrument_definition);
    assert(graph.find_node(first_voice_id)->kind == node_kind::voice_instance);
    assert(graph.find_node(slot_two_id)->kind == node_kind::physical_slot);
    assert(graph.find_node(part_id)->kind == node_kind::part);
    assert(graph.find_node(harmonic_relation_id)->layer == semantic_layer::musical_structure);

    // Flow semantics are not flattened into one event type.
    assert(graph.find_node(part_id)->flow == flow_kind::event);
    assert(graph.find_node(instrument_id)->flow == flow_kind::value);
    assert(graph.find_node(first_voice_id)->flow == flow_kind::stream);

    // Exactness relative to a file is separate from capture completeness.
    const auto& capture_provenance = graph.find_node(captured_vgm_id)->provenance[0];
    assert(capture_provenance.status == evidence_status::exact);
    assert(has_flag(capture_provenance.flags, provenance_flag::runtime_capture));
    assert(has_flag(capture_provenance.flags, provenance_flag::incomplete));

    // Exact source evidence, derived musical identity and hypotheses can coexist.
    assert(graph.find_node(slot_two_id)->attributes[0].status == evidence_status::exact);
    assert(graph.find_node(part_id)->provenance[0].status == evidence_status::derived);
    assert(graph.find_node(harmonic_relation_id)->attributes[0].status == evidence_status::hypothesis);

    // MIDI or other exports are projections of the model, never replacements for it.
    const auto projections = graph.edges_from(part_id, edge_kind::projects_to);
    assert(projections.size() == 1);
    assert(projections[0]->to == midi_projection_id);
    assert(graph.find_node(midi_projection_id)->kind == node_kind::projection);

    // Time coordinates retain their declared clock domain and loop identity.
    const auto& span = *graph.find_node(first_voice_id)->active;
    assert(span.start.domain == time_domain::device);
    assert(span.start.tick_rate == 44100);
    assert(span.start.loop_iteration == 0);
    assert(span.end.has_value());

    // Program structure and the realized musical part are separate objects.
    node process;
    process.kind = node_kind::logical_process;
    process.layer = semantic_layer::driver_execution;
    process.flow = flow_kind::control;
    process.label = "driver track interpreter";
    const auto process_id = graph.add_node(process);

    node loop_entry;
    loop_entry.kind = node_kind::program_point;
    loop_entry.layer = semantic_layer::driver_execution;
    loop_entry.flow = flow_kind::control;
    loop_entry.label = "loop entry";
    loop_entry.provenance.push_back({evidence_status::exact, 1.0, "fixture-driver", 0x200u, "validated driver address"});
    const auto loop_entry_id = graph.add_node(loop_entry);

    node loop_exit = loop_entry;
    loop_exit.label = "loop exit";
    loop_exit.provenance[0].byte_offset = 0x240u;
    const auto loop_exit_id = graph.add_node(loop_exit);

    connect(edge_kind::contains, process_id, loop_entry_id);
    connect(edge_kind::contains, process_id, loop_exit_id);

    edge repeat_flow;
    repeat_flow.kind = edge_kind::control_flows_to;
    repeat_flow.from = loop_exit_id;
    repeat_flow.to = loop_entry_id;
    repeat_flow.attributes.push_back({"transfer", std::string{"repeat"}, evidence_status::exact, 1.0, ""});
    repeat_flow.attributes.push_back({"condition", std::string{"loop_counter > 0"}, evidence_status::exact, 1.0, ""});
    repeat_flow.provenance.push_back({evidence_status::exact, 1.0, "fixture-driver", 0x23eu, "loop branch opcode"});
    const auto repeat_flow_id = graph.add_edge(repeat_flow);

    const auto control_flow = graph.edges_from(loop_exit_id, edge_kind::control_flows_to);
    assert(control_flow.size() == 1);
    assert(control_flow[0]->id == repeat_flow_id);
    assert(control_flow[0]->attributes.size() == 2);
    assert(graph.find_node(process_id)->kind == node_kind::logical_process);
    assert(graph.find_node(loop_entry_id)->kind == node_kind::program_point);
    assert(graph.find_node(part_id)->kind == node_kind::part);

    // One runtime execution is a trace of the static program, not a duplicate program graph.
    node runtime_trace;
    runtime_trace.kind = node_kind::execution_trace;
    runtime_trace.layer = semantic_layer::driver_execution;
    runtime_trace.flow = flow_kind::stream;
    runtime_trace.label = "realized driver execution";
    runtime_trace.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture-runtime",
        std::nullopt,
        "instrumented execution",
        to_flags(provenance_flag::runtime_capture),
    });
    const auto runtime_trace_id = graph.add_node(runtime_trace);

    node first_loop_visit;
    first_loop_visit.kind = node_kind::trace_event;
    first_loop_visit.layer = semantic_layer::driver_execution;
    first_loop_visit.flow = flow_kind::event;
    first_loop_visit.label = "loop entry visit 1";
    first_loop_visit.active = time_span{{time_domain::driver, 120, 60, 0}, std::nullopt};
    first_loop_visit.provenance.push_back({
        evidence_status::exact,
        1.0,
        "fixture-runtime",
        std::nullopt,
        "observed program point",
        to_flags(provenance_flag::runtime_capture),
    });
    const auto first_loop_visit_id = graph.add_node(first_loop_visit);

    node loop_exit_visit = first_loop_visit;
    loop_exit_visit.label = "loop exit visit";
    loop_exit_visit.active = time_span{{time_domain::driver, 180, 60, 0}, std::nullopt};
    const auto loop_exit_visit_id = graph.add_node(loop_exit_visit);

    node second_loop_visit = first_loop_visit;
    second_loop_visit.label = "loop entry visit 2";
    second_loop_visit.active = time_span{{time_domain::driver, 240, 60, 1}, std::nullopt};
    const auto second_loop_visit_id = graph.add_node(second_loop_visit);

    connect(edge_kind::contains, runtime_trace_id, first_loop_visit_id);
    connect(edge_kind::contains, runtime_trace_id, loop_exit_visit_id);
    connect(edge_kind::contains, runtime_trace_id, second_loop_visit_id);
    connect(edge_kind::realizes, loop_entry_id, first_loop_visit_id);
    connect(edge_kind::realizes, loop_exit_id, loop_exit_visit_id);
    connect(edge_kind::realizes, loop_entry_id, second_loop_visit_id);

    const auto trace_contents = graph.edges_from(runtime_trace_id, edge_kind::contains);
    assert(trace_contents.size() == 3);
    assert(graph.find_node(runtime_trace_id)->kind == node_kind::execution_trace);
    assert(has_flag(graph.find_node(runtime_trace_id)->provenance[0].flags, provenance_flag::runtime_capture));

    const auto loop_entry_visits = graph.edges_from(loop_entry_id, edge_kind::realizes);
    assert(loop_entry_visits.size() == 2);
    assert(loop_entry_visits[0]->to != loop_entry_visits[1]->to);
    assert(graph.find_node(loop_entry_visits[0]->to)->kind == node_kind::trace_event);
    assert(graph.find_node(loop_entry_visits[1]->to)->kind == node_kind::trace_event);
    assert(graph.find_node(first_loop_visit_id)->active->start.tick == 120);
    assert(graph.find_node(second_loop_visit_id)->active->start.tick == 240);
    assert(graph.find_node(second_loop_visit_id)->active->start.loop_iteration == 1);

    // Cross-domain timing is an explicit, provenance-bearing relation rather than an implicit cast.
    node authored_note;
    authored_note.kind = node_kind::musical_event;
    authored_note.layer = semantic_layer::authored_program;
    authored_note.flow = flow_kind::event;
    authored_note.label = "authored note";
    authored_note.active = time_span{
        {time_domain::authored, 480, 0, 0},
        time_coordinate{time_domain::authored, 960, 0, 0},
    };
    const auto authored_note_id = graph.add_node(authored_note);

    node acoustic_note;
    acoustic_note.kind = node_kind::acoustic_contribution;
    acoustic_note.layer = semantic_layer::acoustic_realization;
    acoustic_note.flow = flow_kind::stream;
    acoustic_note.label = "rendered note contribution";
    acoustic_note.active = time_span{
        {time_domain::sample, 44100, 44100, 0},
        time_coordinate{time_domain::sample, 66150, 44100, 0},
    };
    const auto acoustic_note_id = graph.add_node(acoustic_note);

    edge alignment;
    alignment.kind = edge_kind::maps_time_to;
    alignment.from = authored_note_id;
    alignment.to = acoustic_note_id;
    alignment.time_map = time_mapping{*authored_note.active, *acoustic_note.active};
    alignment.attributes.push_back({"mapping", std::string{"authored-to-sample"}, evidence_status::derived, 1.0, ""});
    alignment.provenance.push_back({evidence_status::derived, 1.0, "fixture-alignment", std::nullopt, "validated piecewise time mapping"});
    const auto alignment_id = graph.add_edge(alignment);

    const auto time_maps = graph.edges_from(authored_note_id, edge_kind::maps_time_to);
    assert(time_maps.size() == 1);
    assert(time_maps[0]->id == alignment_id);
    assert(time_maps[0]->time_map.has_value());
    assert(time_maps[0]->time_map->from.start.domain == time_domain::authored);
    assert(time_maps[0]->time_map->to.start.domain == time_domain::sample);

    bool rejected_unknown_node = false;
    try {
        edge invalid_relation;
        invalid_relation.kind = edge_kind::causes;
        invalid_relation.from = part_id;
        invalid_relation.to = 999999;
        graph.add_edge(invalid_relation);
    } catch (const std::invalid_argument&) {
        rejected_unknown_node = true;
    }
    assert(rejected_unknown_node);

    bool rejected_missing_time_mapping = false;
    try {
        edge invalid_mapping;
        invalid_mapping.kind = edge_kind::maps_time_to;
        invalid_mapping.from = authored_note_id;
        invalid_mapping.to = acoustic_note_id;
        graph.add_edge(invalid_mapping);
    } catch (const std::invalid_argument&) {
        rejected_missing_time_mapping = true;
    }
    assert(rejected_missing_time_mapping);

    bool rejected_cross_domain_span = false;
    try {
        node invalid_span;
        invalid_span.active = time_span{
            {time_domain::driver, 0, 60, 0},
            time_coordinate{time_domain::sample, 1024, 44100, 0},
        };
        graph.add_node(invalid_span);
    } catch (const std::invalid_argument&) {
        rejected_cross_domain_span = true;
    }
    assert(rejected_cross_domain_span);

    return 0;
}
