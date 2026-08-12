#include "../../components/vgm/enhancement/genesis_execution_graph_adapter.h"

#include <cstdint>
#include <string>

using gameaudio::vgm::append_genesis_trace_event;
using gameaudio::vgm::begin_genesis_execution_trace;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
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

    // Partial payload evidence stays at the source layer. It does not mutate the
    // shadow state or create a guessed device transition.
    const auto before_fnum = handle.shadow.ym2612().channels[0].fnum;
    const std::uint8_t oversized[] = {0xA0, 0x77, 0x99};
    const auto partial = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::command, 130, 0x20D, 0x52, oversized, 3});
    CHECK(!partial.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == before_fnum);
    const node* partial_source = graph.find_node(partial.source_event_id);
    CHECK(partial_source != nullptr);
    CHECK(std::get<bool>(partial_source->attributes[5].value));

    // Reset is an execution-boundary observation. It resets the rebuildable
    // shadow without inventing a hardware register transition that was not in
    // the source command stream.
    const auto reset = append_genesis_trace_event(
        graph,
        handle,
        command_event{command_event_kind::reset, 140, 0, 0, nullptr, 0});
    CHECK(!reset.device_transition_id.has_value());
    CHECK(handle.shadow.ym2612().channels[0].fnum == 0);
    CHECK(handle.source_trace.next_trace_index == 7);

    return 0;
}
