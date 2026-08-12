#include "../../components/vgm/enhancement/genesis_execution_graph_adapter.h"

#include <cstdint>
#include <string>

using gameaudio::vgm::append_genesis_trace_event;
using gameaudio::vgm::begin_genesis_execution_trace;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::command_trace_capture;
using gameaudio::vgm::genesis_state;
using gameaudio::vgm::materialize_genesis_command_capture;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    musical_execution_graph graph;
    auto handle = begin_genesis_execution_trace(
        graph,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    // Same-tick ordered YM2612 writes exercise the real latch/commit semantics:
    // A4 sets the high F-number/block latch, then A0 commits the low byte.
    const std::uint8_t fnum_high[] = {0xA4, 0x2C};
    const std::uint8_t fnum_low[] = {0xA0, 0x34};
    const auto high = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 100, 0x200, 0x52, fnum_high, 2});
    const auto low = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 100, 0x203, 0x52, fnum_low, 2});

    CHECK(high.device_transition_id.has_value());
    CHECK(low.device_transition_id.has_value());
    CHECK(handle.source_trace.next_trace_index == 2);
    CHECK(handle.shadow_continuation_valid);
    CHECK(handle.ym2612_nodes[0].has_value());
    CHECK(!handle.ym2612_nodes[1].has_value());

    const auto& ym = handle.shadow.ym2612();
    CHECK(ym.channels[0].fnum == 0x434);
    CHECK(ym.channels[0].block == 5);

    const node* source_high = graph.find_node(high.source_event_id);
    const node* device_high = graph.find_node(*high.device_transition_id);
    CHECK(source_high != nullptr);
    CHECK(device_high != nullptr);
    CHECK(source_high->layer == semantic_layer::source_representation);
    CHECK(device_high->layer == semantic_layer::synthesis);
    CHECK(device_high->kind == node_kind::trace_event);
    CHECK(device_high->provenance[0].status == evidence_status::derived);
    CHECK(std::get<std::string>(device_high->attributes[0].value) == "YM2612");
    CHECK(std::get<std::uint64_t>(device_high->attributes[3].value) == 0);
    CHECK(std::get<std::uint64_t>(device_high->attributes[4].value) == 0xA4);
    CHECK(std::get<std::uint64_t>(device_high->attributes[5].value) == 0x2C);

    const auto caused = graph.edges_from(high.source_event_id, edge_kind::causes);
    CHECK(caused.size() == 1);
    CHECK(caused[0]->to == *high.device_transition_id);

    const auto ym_transitions = graph.edges_from(*handle.ym2612_nodes[0], edge_kind::contains);
    CHECK(ym_transitions.size() == 2);

    // Key-on changes the derived current snapshot while remaining traceable to
    // an exact source command rather than being promoted to musical-note truth.
    const std::uint8_t key_on[] = {0x28, 0xF0};
    const auto key = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 110, 0x206, 0x52, key_on, 2});
    CHECK(key.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].key_on);
    CHECK(graph.find_node(*key.device_transition_id)->kind == node_kind::trace_event);
    CHECK(graph.find_node(*key.device_transition_id)->kind != node_kind::musical_event);

    // PSG is a separate synthesis object and preserves its own transition state.
    const std::uint8_t psg_latch[] = {0x85};
    const std::uint8_t psg_data[] = {0x12};
    const auto psg_first = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 120, 0x209, 0x50, psg_latch, 1});
    const auto psg_second = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 120, 0x20B, 0x50, psg_data, 1});
    CHECK(psg_first.device_transition_id.has_value());
    CHECK(psg_second.device_transition_id.has_value());
    CHECK(handle.psg_nodes[0].has_value());
    CHECK(handle.shadow.psg().channels[0].tone_period == 0x125);

    // Partial payload evidence stays at the source layer. Because the omitted
    // byte belongs to a supported Genesis command, the exact current shadow can
    // no longer be continued after this observation gap.
    const auto before_fnum = handle.shadow.ym2612().channels[0].fnum;
    const std::uint8_t oversized[] = {0xA0, 0x77, 0x99};
    const auto partial = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 130, 0x20D, 0x52, oversized, 3});
    CHECK(!partial.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == before_fnum);
    CHECK(!handle.shadow_continuation_valid);
    const node* partial_source = graph.find_node(partial.source_event_id);
    CHECK(partial_source != nullptr);
    CHECK(std::get<bool>(partial_source->attributes[5].value));

    // Later complete writes remain exact source observations but cannot be
    // interpreted against a guessed pre-gap latch/register state.
    const auto blocked_high = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 135, 0x210, 0x52, fnum_high, 2});
    const auto blocked_low = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 135, 0x213, 0x52, fnum_low, 2});
    CHECK(!blocked_high.device_transition_id.has_value());
    CHECK(!blocked_low.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == before_fnum);
    CHECK(!handle.shadow_continuation_valid);

    // Reset is an exact resynchronization boundary. It restores a known shadow
    // without claiming that the missing interval was recovered.
    const auto reset = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::reset, 140, 0, 0, nullptr, 0});
    CHECK(!reset.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == 0);
    CHECK(handle.shadow_continuation_valid);

    const auto resynced_high = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 145, 0x216, 0x52, fnum_high, 2});
    const auto resynced_low = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 145, 0x219, 0x52, fnum_low, 2});
    CHECK(resynced_high.device_transition_id.has_value());
    CHECK(resynced_low.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == 0x434);
    CHECK(handle.shadow.ym2612().channels[0].block == 5);
    CHECK(handle.source_trace.next_trace_index == 11);

    // The same semantic lift consumes the allocation-free realtime capture
    // directly, closing the callback -> capture -> graph path.
    command_trace_capture capture;
    capture.begin_window();
    genesis_state realtime_state;
    realtime_state.set_event_tap(&command_trace_capture::tap, &capture);
    realtime_state.observe(command_event{command_event_kind::command, 200, 0x300, 0x52, fnum_high, 2});
    realtime_state.observe(command_event{command_event_kind::command, 200, 0x303, 0x52, fnum_low, 2});
    realtime_state.observe(command_event{command_event_kind::command, 210, 0x306, 0x50, psg_latch, 1});
    realtime_state.observe(command_event{command_event_kind::command, 210, 0x308, 0x50, psg_data, 1});
    CHECK(capture.count() == 4);

    musical_execution_graph capture_graph;
    const auto captured = materialize_genesis_command_capture(
        capture_graph,
        capture,
        "captured.vgm",
        to_flags(provenance_flag::runtime_capture));
    CHECK(captured.source_trace.next_trace_index == 4);
    CHECK(captured.shadow_continuation_valid);
    CHECK(capture_graph.edges_from(captured.source_trace.trace_id, edge_kind::contains).size() == 4);
    CHECK(captured.ym2612_nodes[0].has_value());
    CHECK(captured.psg_nodes[0].has_value());
    CHECK(captured.shadow.ym2612().channels[0].fnum == 0x434);
    CHECK(captured.shadow.ym2612().channels[0].block == 5);
    CHECK(captured.shadow.psg().channels[0].tone_period == 0x125);
    CHECK(capture_graph.edges_from(*captured.ym2612_nodes[0], edge_kind::contains).size() == 2);
    CHECK(capture_graph.edges_from(*captured.psg_nodes[0], edge_kind::contains).size() == 2);

    return 0;
}
