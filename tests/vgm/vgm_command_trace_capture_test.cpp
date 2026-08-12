#include "../../components/vgm/enhancement/genesis_state.h"
#include "../../components/vgm/enhancement/vgm_execution_graph_adapter.h"

#include <cstdint>

using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::command_trace_capture;
using gameaudio::vgm::genesis_state;
using gameaudio::vgm::materialize_vgm_command_capture;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    command_trace_capture capture;
    capture.begin_window();

    genesis_state state;
    state.set_event_tap(&command_trace_capture::tap, &capture);

    const std::uint8_t psg = 0x90;
    const std::uint8_t ym[] = {0x2B, 0x80};
    state.observe(command_event{command_event_kind::command, 10, 0x100, 0x50, &psg, 1});
    state.observe(command_event{command_event_kind::command, 20, 0x110, 0x52, ym, 2});
    state.observe(command_event{command_event_kind::reset, 30, 0, 0x00, nullptr, 0});
    state.observe(command_event{command_event_kind::command, 40, 0x120, 0x50, &psg, 1});

    // genesis_state preserves its tap across reset, so the bounded capture sees
    // the complete observed window without allocating in the callback.
    CHECK(capture.count() == 4);
    CHECK(!capture.overflowed());
    CHECK(capture.dropped() == 0);
    CHECK(capture.records()[0].tick == 10);
    CHECK(capture.records()[0].file_offset == 0x100);
    CHECK(capture.records()[1].command == 0x52);
    CHECK(capture.records()[1].payload_size == 2);
    CHECK(capture.records()[2].kind == command_event_kind::reset);
    CHECK(capture.records()[3].tick == 40);

    musical_execution_graph graph;
    const auto trace = materialize_vgm_command_capture(
        graph,
        capture,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    const node* trace_node = graph.find_node(trace.trace_id);
    CHECK(trace_node != nullptr);
    CHECK(trace_node->kind == node_kind::execution_trace);
    CHECK(!has_flag(trace_node->provenance[0].flags, provenance_flag::incomplete));
    CHECK(graph.edges_from(trace.trace_id, edge_kind::contains).size() == 4);

    // A bounded capture never silently overwrites or reallocates. Excess events
    // are counted and become incomplete provenance when materialized.
    capture.begin_window();
    const command_event repeated{command_event_kind::command, 50, 0x130, 0x50, &psg, 1};
    for (std::size_t i = 0; i < command_trace_capture::capacity + 2; ++i)
        capture.observe(repeated);

    CHECK(capture.count() == command_trace_capture::capacity);
    CHECK(capture.overflowed());
    CHECK(capture.dropped() == 2);

    musical_execution_graph overflow_graph;
    const auto overflow_trace = materialize_vgm_command_capture(
        overflow_graph,
        capture,
        "overflow.vgm",
        to_flags(provenance_flag::runtime_capture));
    const node* overflow_node = overflow_graph.find_node(overflow_trace.trace_id);
    CHECK(overflow_node != nullptr);
    CHECK(has_flag(overflow_node->provenance[0].flags, provenance_flag::incomplete));
    CHECK(overflow_node->attributes.size() == 2);
    CHECK(std::get<bool>(overflow_node->attributes[0].value));
    CHECK(std::get<std::uint64_t>(overflow_node->attributes[1].value) == 2);
    CHECK(overflow_graph.edges_from(overflow_trace.trace_id, edge_kind::contains).size() == command_trace_capture::capacity);

    return 0;
}
