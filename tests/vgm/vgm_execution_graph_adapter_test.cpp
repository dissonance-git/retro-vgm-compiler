#include "../../components/vgm/enhancement/vgm_execution_graph_adapter.h"

#include <cstdint>
#include <stdexcept>
#include <string>

using gameaudio::vgm::append_vgm_trace_event;
using gameaudio::vgm::begin_vgm_execution_trace;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::vgm_execution_trace_handle;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    musical_execution_graph graph;
    const provenance_flags capture_flags =
        provenance_flag::runtime_capture | provenance_flag::incomplete;
    auto trace = begin_vgm_execution_trace(graph, "fixture.vgm", capture_flags);

    const std::uint8_t psg[] = {0x90};
    const std::uint8_t ym[] = {0x2B, 0x80};
    const std::uint8_t dac[] = {0x7F};

    // Equal source timestamps remain ordered by exact parser observation order.
    const auto psg_event_id = append_vgm_trace_event(
        graph,
        trace,
        command_event{command_event_kind::command, 10, 0x100, 0x50, psg, 1});
    const auto ym_event_id = append_vgm_trace_event(
        graph,
        trace,
        command_event{command_event_kind::command, 10, 0x110, 0x52, ym, 2});
    const auto dac_event_id = append_vgm_trace_event(
        graph,
        trace,
        command_event{command_event_kind::ym2612_dac, 30, 0x120, 0x00, dac, 1});
    const auto reset_event_id = append_vgm_trace_event(
        graph,
        trace,
        command_event{command_event_kind::reset, 40, 0, 0x00, nullptr, 0});

    const node* trace_node = graph.find_node(trace.trace_id);
    CHECK(trace_node != nullptr);
    CHECK(trace_node->kind == node_kind::execution_trace);
    CHECK(trace_node->layer == semantic_layer::source_representation);
    CHECK(has_flag(trace_node->provenance[0].flags, provenance_flag::runtime_capture));
    CHECK(has_flag(trace_node->provenance[0].flags, provenance_flag::incomplete));
    CHECK(trace.next_trace_index == 4);

    const auto contents = graph.edges_from(trace.trace_id, edge_kind::contains);
    CHECK(contents.size() == 4);

    const node* psg_event = graph.find_node(psg_event_id);
    CHECK(psg_event != nullptr);
    CHECK(psg_event->kind == node_kind::trace_event);
    CHECK(psg_event->kind != node_kind::musical_event);
    CHECK(psg_event->layer == semantic_layer::source_representation);
    CHECK(psg_event->active.has_value());
    CHECK(psg_event->active->start.domain == time_domain::source);
    CHECK(psg_event->active->start.tick == 10);
    CHECK(psg_event->active->start.tick_rate == 0);
    CHECK(psg_event->provenance[0].byte_offset.has_value());
    CHECK(*psg_event->provenance[0].byte_offset == 0x100);
    CHECK(std::get<std::string>(psg_event->attributes[0].value) == "command");
    CHECK(std::get<std::uint64_t>(psg_event->attributes[1].value) == 0x50);
    CHECK(std::get<std::uint64_t>(psg_event->attributes[2].value) == 1);
    CHECK(std::get<std::uint64_t>(psg_event->attributes[3].value) == 0);
    CHECK(std::get<std::uint64_t>(psg_event->attributes[4].value) == 1);
    CHECK(!std::get<bool>(psg_event->attributes[5].value));
    CHECK(std::get<std::uint64_t>(psg_event->attributes[6].value) == 0x90);

    const node* ym_event = graph.find_node(ym_event_id);
    CHECK(ym_event != nullptr);
    CHECK(ym_event->active->start.tick == 10);
    CHECK(std::get<std::uint64_t>(ym_event->attributes[1].value) == 0x52);
    CHECK(std::get<std::uint64_t>(ym_event->attributes[2].value) == 2);
    CHECK(std::get<std::uint64_t>(ym_event->attributes[3].value) == 1);
    CHECK(std::get<std::uint64_t>(ym_event->attributes[4].value) == 2);
    CHECK(!std::get<bool>(ym_event->attributes[5].value));
    CHECK(std::get<std::uint64_t>(ym_event->attributes[6].value) == 0x2B);
    CHECK(std::get<std::uint64_t>(ym_event->attributes[7].value) == 0x80);

    const node* dac_event = graph.find_node(dac_event_id);
    CHECK(dac_event != nullptr);
    CHECK(std::get<std::string>(dac_event->attributes[0].value) == "ym2612_dac");
    CHECK(std::get<std::uint64_t>(dac_event->attributes[3].value) == 2);

    const node* reset_event = graph.find_node(reset_event_id);
    CHECK(reset_event != nullptr);
    CHECK(std::get<std::string>(reset_event->attributes[0].value) == "reset");
    CHECK(std::get<std::uint64_t>(reset_event->attributes[3].value) == 3);
    CHECK(std::get<std::uint64_t>(reset_event->attributes[4].value) == 0);
    CHECK(!std::get<bool>(reset_event->attributes[5].value));
    CHECK(!reset_event->provenance[0].byte_offset.has_value());

    bool rejected_invalid_trace = false;
    try {
        vgm_execution_trace_handle invalid;
        invalid.trace_id = 999999;
        invalid.source = "fixture.vgm";
        invalid.provenance_flags = capture_flags;
        append_vgm_trace_event(
            graph,
            invalid,
            command_event{command_event_kind::command, 50, 0x130, 0x50, psg, 1});
    } catch (const std::invalid_argument&) {
        rejected_invalid_trace = true;
    }
    CHECK(rejected_invalid_trace);

    return 0;
}
