#include "../../components/vgm/enhancement/genesis_performance_adapter.h"

#include <cstdint>
#include <string>

using gameaudio::vgm::append_genesis_performance_event;
using gameaudio::vgm::begin_genesis_performance_trace;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    musical_execution_graph graph;
    auto handle = begin_genesis_performance_trace(
        graph,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    const std::uint8_t fnum_high[] = {0xA4, 0x2C};
    const std::uint8_t fnum_low[] = {0xA0, 0x34};
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 10, 0x100, 0x52, fnum_high, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 10, 0x103, 0x52, fnum_low, 2});

    // Ordinary full four-operator gate with a known channel pitch becomes a
    // conservative pitched-activity observation, not a MIDI note or part.
    const std::uint8_t full_key_on[] = {0x28, 0xF0};
    const auto ym_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 20, 0x106, 0x52, full_key_on, 2});
    CHECK(ym_on.execution.device_transition_id.has_value());
    CHECK(ym_on.performance_event_id.has_value());

    const node* ym_on_event = graph.find_node(*ym_on.performance_event_id);
    CHECK(ym_on_event != nullptr);
    CHECK(ym_on_event->kind == node_kind::musical_event);
    CHECK(ym_on_event->layer == semantic_layer::musical_performance);
    CHECK(ym_on_event->provenance[0].status == evidence_status::derived);
    CHECK(std::get<std::string>(ym_on_event->attributes[0].value) == "pitched_activity_onset");
    CHECK(std::get<std::string>(ym_on_event->attributes[1].value) == "YM2612");
    CHECK(std::get<std::uint64_t>(ym_on_event->attributes[3].value) == 0);
    CHECK(std::get<std::string>(ym_on_event->attributes[4].value) == "unresolved");
    CHECK(std::get<std::uint64_t>(ym_on_event->attributes[5].value) == 0x434);
    CHECK(std::get<std::uint64_t>(ym_on_event->attributes[6].value) == 5);
    CHECK(std::get<std::uint64_t>(ym_on_event->attributes[7].value) == 0x0F);

    const auto performance_support = graph.edges_from(
        *ym_on.execution.device_transition_id,
        edge_kind::derived_from);
    CHECK(performance_support.size() == 1);
    CHECK(performance_support[0]->to == *ym_on.performance_event_id);

    // Pitch change while the gate remains open is device/performance control
    // evidence, but this first slice does not force a bend/retrigger decision.
    const std::uint8_t pitch_low_changed[] = {0xA0, 0x44};
    const auto ym_pitch = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 25, 0x109, 0x52, pitch_low_changed, 2});
    CHECK(ym_pitch.execution.device_transition_id.has_value());
    CHECK(!ym_pitch.performance_event_id.has_value());

    const std::uint8_t full_key_off[] = {0x28, 0x00};
    const auto ym_off = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 30, 0x10C, 0x52, full_key_off, 2});
    CHECK(ym_off.performance_event_id.has_value());
    CHECK(std::get<std::string>(graph.find_node(*ym_off.performance_event_id)->attributes[0].value) ==
          "pitched_activity_release");

    // Any-nonzero key-mask -> MIDI NoteOn is too aggressive. Partial operator
    // re-keying stays at the device layer until algorithm/carrier context proves
    // a higher musical interpretation.
    const std::uint8_t partial_key_on[] = {0x28, 0x10};
    const auto partial_key = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 40, 0x10F, 0x52, partial_key_on, 2});
    CHECK(partial_key.execution.device_transition_id.has_value());
    CHECK(!partial_key.performance_event_id.has_value());
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 41, 0x112, 0x52, full_key_off, 2});

    // Channel 3 special mode can give operators independent pitch behavior, so
    // a channel-level note-like event is intentionally withheld.
    const std::uint8_t ch3_high[] = {0xA6, 0x2C};
    const std::uint8_t ch3_low[] = {0xA2, 0x34};
    const std::uint8_t ch3_special[] = {0x27, 0x40};
    const std::uint8_t ch3_key_on[] = {0x28, 0xF2};
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 50, 0x115, 0x52, ch3_high, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 50, 0x118, 0x52, ch3_low, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 51, 0x11B, 0x52, ch3_special, 2});
    const auto ch3_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 52, 0x11E, 0x52, ch3_key_on, 2});
    CHECK(ch3_on.execution.device_transition_id.has_value());
    CHECK(!ch3_on.performance_event_id.has_value());

    // DAC mode replaces ordinary FM-channel semantics for channel 6.
    const std::uint8_t ch6_high[] = {0xA6, 0x2C};
    const std::uint8_t ch6_low[] = {0xA2, 0x34};
    const std::uint8_t dac_enable[] = {0x2B, 0x80};
    const std::uint8_t ch6_key_on[] = {0x28, 0xF6};
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 60, 0x121, 0x53, ch6_high, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 60, 0x124, 0x53, ch6_low, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 61, 0x127, 0x52, dac_enable, 2});
    const auto ch6_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 62, 0x12A, 0x52, ch6_key_on, 2});
    CHECK(ch6_on.execution.device_transition_id.has_value());
    CHECK(!ch6_on.performance_event_id.has_value());

    // PSG tone activity is reconstructed from state, not from one write in
    // isolation: pitch must exist, the channel must be routed, and attenuation
    // must cross the hardware mute boundary.
    const std::uint8_t psg_tone_latch = 0x85;
    const std::uint8_t psg_tone_data = 0x12;
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 70, 0x12D, 0x50, &psg_tone_latch, 1});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 70, 0x12F, 0x50, &psg_tone_data, 1});

    const std::uint8_t psg_unmute = 0x90;
    const auto psg_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 71, 0x131, 0x50, &psg_unmute, 1});
    CHECK(psg_on.performance_event_id.has_value());
    const node* psg_on_event = graph.find_node(*psg_on.performance_event_id);
    CHECK(psg_on_event != nullptr);
    CHECK(std::get<std::string>(psg_on_event->attributes[0].value) == "pitched_activity_onset");
    CHECK(std::get<std::string>(psg_on_event->attributes[1].value) == "SN76489");
    CHECK(std::get<std::uint64_t>(psg_on_event->attributes[5].value) == 0x125);

    const std::uint8_t psg_level_change = 0x94;
    const auto psg_level = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 72, 0x133, 0x50, &psg_level_change, 1});
    CHECK(psg_level.execution.device_transition_id.has_value());
    CHECK(!psg_level.performance_event_id.has_value());

    const std::uint8_t psg_mute = 0x9F;
    const auto psg_off = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 73, 0x135, 0x50, &psg_mute, 1});
    CHECK(psg_off.performance_event_id.has_value());
    CHECK(std::get<std::string>(graph.find_node(*psg_off.performance_event_id)->attributes[0].value) ==
          "pitched_activity_release");

    // Noise-channel gating is not forced into a pitched-note ontology.
    const std::uint8_t noise_control = 0xE4;
    const std::uint8_t noise_unmute = 0xF0;
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 80, 0x137, 0x50, &noise_control, 1});
    const auto noise_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 81, 0x139, 0x50, &noise_unmute, 1});
    CHECK(noise_on.execution.device_transition_id.has_value());
    CHECK(!noise_on.performance_event_id.has_value());

    // The bridge has intentionally not invented persistent musical parts or a
    // MIDI projection merely because physical activity was recoverable.
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());
    CHECK(graph.nodes_of_kind(node_kind::musical_event).size() == 4);

    return 0;
}
