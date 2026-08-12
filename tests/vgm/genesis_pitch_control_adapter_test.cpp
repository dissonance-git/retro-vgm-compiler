#include "../../components/vgm/enhancement/genesis_pitch_control_adapter.h"

#include <cstdint>
#include <string>

using gameaudio::vgm::append_genesis_pitch_control_event;
using gameaudio::vgm::begin_genesis_pitch_control_trace;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    musical_execution_graph graph;
    auto handle = begin_genesis_pitch_control_trace(
        graph,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    const std::uint8_t fnum_high[] = {0xA4, 0x2C};
    const std::uint8_t fnum_low[] = {0xA0, 0x34};
    const auto high = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 10, 0x100, 0x52, fnum_high, 2});
    const auto low = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 10, 0x103, 0x52, fnum_low, 2});
    CHECK(high.performance.execution.device_transition_id.has_value());
    CHECK(low.performance.execution.device_transition_id.has_value());
    CHECK(handle.ym_last_pitch_transition[0][0].has_value());
    CHECK(*handle.ym_last_pitch_transition[0][0] == *low.performance.execution.device_transition_id);

    const std::uint8_t key_on[] = {0x28, 0xF0};
    const auto onset = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 20, 0x106, 0x52, key_on, 2});
    CHECK(onset.performance.performance_event_id.has_value());
    CHECK(onset.performance.physical_voice_episode_id.has_value());
    CHECK(onset.pitch_parameter_id.has_value());
    CHECK(onset.pitch_support_edge_id.has_value());
    CHECK(handle.ym_pitch_parameter[0][0].has_value());
    const node_id ym_parameter_id = *handle.ym_pitch_parameter[0][0];
    CHECK(ym_parameter_id == *onset.pitch_parameter_id);

    const node* ym_parameter = graph.find_node(ym_parameter_id);
    CHECK(ym_parameter != nullptr);
    CHECK(ym_parameter->kind == node_kind::parameter);
    CHECK(ym_parameter->layer == semantic_layer::musical_performance);
    CHECK(ym_parameter->flow == flow_kind::control);
    CHECK(ym_parameter->active.has_value());
    CHECK(ym_parameter->active->start.tick == 20);
    CHECK(!ym_parameter->active->end.has_value());
    CHECK(std::get<std::string>(ym_parameter->attributes[0].value) == "pitch");
    CHECK(std::get<std::string>(ym_parameter->attributes[1].value) == "device_native");
    CHECK(std::get<std::string>(ym_parameter->attributes[2].value) == "YM2612");
    CHECK(std::get<std::string>(ym_parameter->attributes[5].value) == "unresolved");

    const auto controls = graph.edges_from(ym_parameter_id, edge_kind::controls);
    CHECK(controls.size() == 1);
    CHECK(controls[0]->to == *onset.performance.physical_voice_episode_id);

    const auto initial_support = graph.edges_to(ym_parameter_id, edge_kind::contributes_to);
    CHECK(initial_support.size() == 1);
    CHECK(initial_support[0]->from == *low.performance.execution.device_transition_id);
    CHECK(initial_support[0]->active.has_value());
    CHECK(initial_support[0]->active->start.tick == 10);
    CHECK(std::get<std::string>(initial_support[0]->attributes[0].value) == "initial_state");
    CHECK(std::get<std::uint64_t>(initial_support[0]->attributes[1].value) == 0x434);
    CHECK(std::get<std::uint64_t>(initial_support[0]->attributes[2].value) == 5);

    // Device-native pitch changes remain one control history. They do not force
    // a MIDI pitch-bend vs note-retrigger decision.
    const std::uint8_t changed_low[] = {0xA0, 0x44};
    const auto pitch_change = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 25, 0x109, 0x52, changed_low, 2});
    CHECK(pitch_change.performance.execution.device_transition_id.has_value());
    CHECK(!pitch_change.performance.performance_event_id.has_value());
    CHECK(pitch_change.pitch_parameter_id.has_value());
    CHECK(*pitch_change.pitch_parameter_id == ym_parameter_id);
    CHECK(pitch_change.pitch_support_edge_id.has_value());
    CHECK(handle.ym_pitch_parameter[0][0].has_value());
    CHECK(*handle.ym_pitch_parameter[0][0] == ym_parameter_id);

    const auto changed_support = graph.edges_to(ym_parameter_id, edge_kind::contributes_to);
    CHECK(changed_support.size() == 2);
    CHECK(changed_support[1]->from == *pitch_change.performance.execution.device_transition_id);
    CHECK(changed_support[1]->active->start.tick == 25);
    CHECK(std::get<std::string>(changed_support[1]->attributes[0].value) == "state_change");
    CHECK(std::get<std::uint64_t>(changed_support[1]->attributes[1].value) == 0x444);
    CHECK(std::get<std::uint64_t>(changed_support[1]->attributes[2].value) == 5);

    const std::uint8_t partial_rekey[] = {0x28, 0x70};
    const auto partial = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 26, 0x10C, 0x52, partial_rekey, 2});
    CHECK(!partial.performance.performance_event_id.has_value());
    CHECK(!partial.pitch_support_edge_id.has_value());
    CHECK(graph.edges_to(ym_parameter_id, edge_kind::contributes_to).size() == 2);

    const std::uint8_t key_off[] = {0x28, 0x00};
    const auto release = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 30, 0x10F, 0x52, key_off, 2});
    CHECK(release.performance.performance_event_id.has_value());
    CHECK(!handle.ym_pitch_parameter[0][0].has_value());
    ym_parameter = graph.find_node(ym_parameter_id);
    CHECK(ym_parameter->active->end.has_value());
    CHECK(ym_parameter->active->end->tick == 30);
    CHECK(ym_parameter->attributes.size() == 7);
    CHECK(ym_parameter->attributes[6].key == "termination_reason");
    CHECK(std::get<std::string>(ym_parameter->attributes[6].value) == "pitched_activity_release");

    // SN76489 gets the same representation without pretending its period is the
    // same unit as YM2612 F-number/block.
    const std::uint8_t psg_latch = 0x85;
    const std::uint8_t psg_data = 0x12;
    append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 40, 0x120, 0x50, &psg_latch, 1});
    const auto psg_pitch = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 40, 0x122, 0x50, &psg_data, 1});
    CHECK(psg_pitch.performance.execution.device_transition_id.has_value());
    CHECK(handle.psg_last_pitch_transition[0][0].has_value());

    const std::uint8_t psg_unmute = 0x90;
    const auto psg_on = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 41, 0x124, 0x50, &psg_unmute, 1});
    CHECK(psg_on.performance.performance_event_id.has_value());
    CHECK(psg_on.pitch_parameter_id.has_value());
    const node_id psg_parameter_id = *psg_on.pitch_parameter_id;
    CHECK(handle.psg_pitch_parameter[0][0].has_value());
    CHECK(*handle.psg_pitch_parameter[0][0] == psg_parameter_id);
    const auto psg_initial_support = graph.edges_to(psg_parameter_id, edge_kind::contributes_to);
    CHECK(psg_initial_support.size() == 1);
    CHECK(psg_initial_support[0]->from == *psg_pitch.performance.execution.device_transition_id);
    CHECK(std::get<std::uint64_t>(psg_initial_support[0]->attributes[1].value) == 0x125);
    CHECK(psg_initial_support[0]->attributes.size() == 2);

    // Re-latch tone then change its upper data bits while the episode remains
    // open. The parameter gains another state change, not another note event.
    append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 42, 0x126, 0x50, &psg_latch, 1});
    const std::uint8_t psg_data_changed = 0x13;
    const auto psg_change = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 42, 0x128, 0x50, &psg_data_changed, 1});
    CHECK(psg_change.performance.execution.device_transition_id.has_value());
    CHECK(!psg_change.performance.performance_event_id.has_value());
    CHECK(psg_change.pitch_parameter_id.has_value());
    CHECK(*psg_change.pitch_parameter_id == psg_parameter_id);
    const auto psg_changed_support = graph.edges_to(psg_parameter_id, edge_kind::contributes_to);
    CHECK(psg_changed_support.size() == 2);
    CHECK(std::get<std::uint64_t>(psg_changed_support[1]->attributes[1].value) == 0x135);

    const std::uint8_t psg_mute = 0x9F;
    const auto psg_off = append_genesis_pitch_control_event(
        graph,
        handle,
        command_event{command_event_kind::command, 43, 0x12A, 0x50, &psg_mute, 1});
    CHECK(psg_off.performance.performance_event_id.has_value());
    CHECK(!handle.psg_pitch_parameter[0][0].has_value());
    const node* psg_parameter = graph.find_node(psg_parameter_id);
    CHECK(psg_parameter != nullptr);
    CHECK(psg_parameter->active->end.has_value());
    CHECK(psg_parameter->active->end->tick == 43);
    CHECK(std::get<std::string>(psg_parameter->attributes[6].value) == "pitched_activity_release");

    // No generic musical part, MIDI projection, or interpolated curve was
    // invented. The graph contains bounded control objects over source-backed
    // device-state changes.
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());
    CHECK(graph.nodes_of_kind(node_kind::parameter).size() == 2);

    return 0;
}
