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

    // Ordinary full four-operator gate with a known channel pitch creates both
    // a conservative performance observation and a bounded physical synthesis
    // episode. Neither object is a persistent musical part.
    const std::uint8_t full_key_on[] = {0x28, 0xF0};
    const auto ym_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 20, 0x106, 0x52, full_key_on, 2});
    CHECK(ym_on.execution.device_transition_id.has_value());
    CHECK(ym_on.performance_event_id.has_value());
    CHECK(ym_on.physical_voice_episode_id.has_value());
    CHECK(handle.ym_physical_voice_episode[0][0].has_value());
    CHECK(*handle.ym_physical_voice_episode[0][0] == *ym_on.physical_voice_episode_id);
    const node_id ym_episode_id = *ym_on.physical_voice_episode_id;

    const node* ym_episode = graph.find_node(ym_episode_id);
    CHECK(ym_episode != nullptr);
    CHECK(ym_episode->kind == node_kind::voice_instance);
    CHECK(ym_episode->layer == semantic_layer::synthesis);
    CHECK(ym_episode->active.has_value());
    CHECK(ym_episode->active->start.tick == 20);
    CHECK(!ym_episode->active->end.has_value());
    CHECK(std::get<std::string>(ym_episode->attributes[0].value) == "YM2612");
    CHECK(std::get<std::uint64_t>(ym_episode->attributes[2].value) == 0);
    CHECK(std::get<std::string>(ym_episode->attributes[3].value) == "physical_voice_episode");
    CHECK(std::get<std::string>(ym_episode->attributes[4].value) == "unresolved");

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
    const auto ym_realizations = graph.edges_from(*ym_on.performance_event_id, edge_kind::realizes);
    CHECK(ym_realizations.size() == 1);
    CHECK(ym_realizations[0]->to == ym_episode_id);

    // Pitch change while the gate remains open is device/performance control
    // evidence, but this slice does not force a bend/retrigger decision and the
    // already-open physical episode keeps its identity.
    const std::uint8_t pitch_low_changed[] = {0xA0, 0x44};
    const auto ym_pitch = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 25, 0x109, 0x52, pitch_low_changed, 2});
    CHECK(ym_pitch.execution.device_transition_id.has_value());
    CHECK(!ym_pitch.performance_event_id.has_value());
    CHECK(handle.ym_physical_voice_episode[0][0].has_value());
    CHECK(*handle.ym_physical_voice_episode[0][0] == ym_episode_id);

    // Partial operator re-keying during an already proved activity episode does
    // not create another onset and does not invent a second physical identity.
    const std::uint8_t partial_during_open[] = {0x28, 0x70};
    const auto ym_partial_during = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 26, 0x10C, 0x52, partial_during_open, 2});
    CHECK(ym_partial_during.execution.device_transition_id.has_value());
    CHECK(!ym_partial_during.performance_event_id.has_value());
    CHECK(handle.ym_physical_voice_episode[0][0].has_value());
    CHECK(*handle.ym_physical_voice_episode[0][0] == ym_episode_id);

    const std::uint8_t full_key_off[] = {0x28, 0x00};
    const auto ym_off = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 30, 0x10F, 0x52, full_key_off, 2});
    CHECK(ym_off.performance_event_id.has_value());
    CHECK(ym_off.physical_voice_episode_id.has_value());
    CHECK(*ym_off.physical_voice_episode_id == ym_episode_id);
    CHECK(!handle.ym_physical_voice_episode[0][0].has_value());
    CHECK(std::get<std::string>(graph.find_node(*ym_off.performance_event_id)->attributes[0].value) ==
          "pitched_activity_release");

    ym_episode = graph.find_node(ym_episode_id);
    CHECK(ym_episode != nullptr);
    CHECK(ym_episode->active->end.has_value());
    CHECK(ym_episode->active->end->tick == 30);
    CHECK(std::get<std::string>(ym_episode->attributes[5].value) == "pitched_activity_release");
    const auto ym_release_realizations = graph.edges_from(*ym_off.performance_event_id, edge_kind::realizes);
    CHECK(ym_release_realizations.size() == 1);
    CHECK(ym_release_realizations[0]->to == ym_episode_id);

    // Any-nonzero key-mask -> MIDI NoteOn is too aggressive. Partial operator
    // re-keying from silence stays at the device layer and creates no physical
    // episode until stronger evidence supports one.
    const std::uint8_t partial_key_on[] = {0x28, 0x10};
    const auto partial_key = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 40, 0x112, 0x52, partial_key_on, 2});
    CHECK(partial_key.execution.device_transition_id.has_value());
    CHECK(!partial_key.performance_event_id.has_value());
    CHECK(!partial_key.physical_voice_episode_id.has_value());
    CHECK(!handle.ym_physical_voice_episode[0][0].has_value());
    const auto partial_key_off = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 41, 0x115, 0x52, full_key_off, 2});
    CHECK(!partial_key_off.performance_event_id.has_value());
    CHECK(!partial_key_off.physical_voice_episode_id.has_value());

    // Channel 3 special mode can give operators independent pitch behavior, so
    // a simple channel-level physical episode is intentionally withheld.
    const std::uint8_t ch3_high[] = {0xA6, 0x2C};
    const std::uint8_t ch3_low[] = {0xA2, 0x34};
    const std::uint8_t ch3_special[] = {0x27, 0x40};
    const std::uint8_t ch3_key_on[] = {0x28, 0xF2};
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 50, 0x118, 0x52, ch3_high, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 50, 0x11B, 0x52, ch3_low, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 51, 0x11E, 0x52, ch3_special, 2});
    const auto ch3_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 52, 0x121, 0x52, ch3_key_on, 2});
    CHECK(ch3_on.execution.device_transition_id.has_value());
    CHECK(!ch3_on.performance_event_id.has_value());
    CHECK(!ch3_on.physical_voice_episode_id.has_value());
    CHECK(!handle.ym_physical_voice_episode[0][2].has_value());

    // DAC mode replaces ordinary FM-channel semantics for channel 6.
    const std::uint8_t ch6_high[] = {0xA6, 0x2C};
    const std::uint8_t ch6_low[] = {0xA2, 0x34};
    const std::uint8_t dac_enable[] = {0x2B, 0x80};
    const std::uint8_t ch6_key_on[] = {0x28, 0xF6};
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 60, 0x124, 0x53, ch6_high, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 60, 0x127, 0x53, ch6_low, 2});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 61, 0x12A, 0x52, dac_enable, 2});
    const auto ch6_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 62, 0x12D, 0x52, ch6_key_on, 2});
    CHECK(ch6_on.execution.device_transition_id.has_value());
    CHECK(!ch6_on.performance_event_id.has_value());
    CHECK(!ch6_on.physical_voice_episode_id.has_value());
    CHECK(!handle.ym_physical_voice_episode[0][5].has_value());

    // PSG tone activity is reconstructed from state, not from one write in
    // isolation: pitch must exist, the channel must be routed, and attenuation
    // must cross the hardware mute boundary.
    const std::uint8_t psg_tone_latch = 0x85;
    const std::uint8_t psg_tone_data = 0x12;
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 70, 0x130, 0x50, &psg_tone_latch, 1});
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 70, 0x132, 0x50, &psg_tone_data, 1});

    const std::uint8_t psg_unmute = 0x90;
    const auto psg_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 71, 0x134, 0x50, &psg_unmute, 1});
    CHECK(psg_on.performance_event_id.has_value());
    CHECK(psg_on.physical_voice_episode_id.has_value());
    CHECK(handle.psg_physical_voice_episode[0][0].has_value());
    CHECK(*handle.psg_physical_voice_episode[0][0] == *psg_on.physical_voice_episode_id);
    const node_id psg_episode_id = *psg_on.physical_voice_episode_id;
    const node* psg_episode = graph.find_node(psg_episode_id);
    CHECK(psg_episode != nullptr);
    CHECK(psg_episode->kind == node_kind::voice_instance);
    CHECK(psg_episode->active->start.tick == 71);
    CHECK(std::get<std::string>(psg_episode->attributes[0].value) == "SN76489");

    const node* psg_on_event = graph.find_node(*psg_on.performance_event_id);
    CHECK(psg_on_event != nullptr);
    CHECK(std::get<std::string>(psg_on_event->attributes[0].value) == "pitched_activity_onset");
    CHECK(std::get<std::string>(psg_on_event->attributes[1].value) == "SN76489");
    CHECK(std::get<std::uint64_t>(psg_on_event->attributes[5].value) == 0x125);
    const auto psg_realizations = graph.edges_from(*psg_on.performance_event_id, edge_kind::realizes);
    CHECK(psg_realizations.size() == 1);
    CHECK(psg_realizations[0]->to == psg_episode_id);

    const std::uint8_t psg_level_change = 0x94;
    const auto psg_level = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 72, 0x136, 0x50, &psg_level_change, 1});
    CHECK(psg_level.execution.device_transition_id.has_value());
    CHECK(!psg_level.performance_event_id.has_value());
    CHECK(handle.psg_physical_voice_episode[0][0].has_value());
    CHECK(*handle.psg_physical_voice_episode[0][0] == psg_episode_id);

    const std::uint8_t psg_mute = 0x9F;
    const auto psg_off = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 73, 0x138, 0x50, &psg_mute, 1});
    CHECK(psg_off.performance_event_id.has_value());
    CHECK(psg_off.physical_voice_episode_id.has_value());
    CHECK(*psg_off.physical_voice_episode_id == psg_episode_id);
    CHECK(!handle.psg_physical_voice_episode[0][0].has_value());
    CHECK(std::get<std::string>(graph.find_node(*psg_off.performance_event_id)->attributes[0].value) ==
          "pitched_activity_release");
    psg_episode = graph.find_node(psg_episode_id);
    CHECK(psg_episode->active->end.has_value());
    CHECK(psg_episode->active->end->tick == 73);
    CHECK(std::get<std::string>(psg_episode->attributes[5].value) == "pitched_activity_release");

    // Noise-channel gating is not forced into a pitched-note ontology or a tone
    // voice episode.
    const std::uint8_t noise_control = 0xE4;
    const std::uint8_t noise_unmute = 0xF0;
    append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 80, 0x13A, 0x50, &noise_control, 1});
    const auto noise_on = append_genesis_performance_event(
        graph,
        handle,
        command_event{command_event_kind::command, 81, 0x13C, 0x50, &noise_unmute, 1});
    CHECK(noise_on.execution.device_transition_id.has_value());
    CHECK(!noise_on.performance_event_id.has_value());
    CHECK(!noise_on.physical_voice_episode_id.has_value());

    // A gate that opens before pitch is known never earns an onset merely because
    // a later frequency write makes the channel legible. It also never opens a
    // physical episode, so the later key-off cannot create an orphan release.
    auto no_pitch = begin_genesis_performance_trace(
        graph,
        "no-pitch.vgm",
        to_flags(provenance_flag::runtime_capture));
    const auto early_gate = append_genesis_performance_event(
        graph,
        no_pitch,
        command_event{command_event_kind::command, 90, 0x200, 0x52, full_key_on, 2});
    CHECK(!early_gate.performance_event_id.has_value());
    CHECK(!early_gate.physical_voice_episode_id.has_value());
    CHECK(!no_pitch.ym_physical_voice_episode[0][0].has_value());
    const auto late_high = append_genesis_performance_event(
        graph,
        no_pitch,
        command_event{command_event_kind::command, 91, 0x203, 0x52, fnum_high, 2});
    const auto late_low = append_genesis_performance_event(
        graph,
        no_pitch,
        command_event{command_event_kind::command, 91, 0x206, 0x52, fnum_low, 2});
    CHECK(!late_high.performance_event_id.has_value());
    CHECK(!late_low.performance_event_id.has_value());
    const auto orphan_guard = append_genesis_performance_event(
        graph,
        no_pitch,
        command_event{command_event_kind::command, 92, 0x209, 0x52, full_key_off, 2});
    CHECK(!orphan_guard.performance_event_id.has_value());
    CHECK(!orphan_guard.physical_voice_episode_id.has_value());

    // Main-path physical episodes are deliberately bounded synthesis identities.
    // No persistent part or MIDI projection has been invented from them.
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());
    CHECK(graph.nodes_of_kind(node_kind::musical_event).size() == 4);
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).size() == 2);

    // Leaving the simple interpretation subset closes an already-proved physical
    // episode at the exact boundary but does not manufacture a musical release.
    musical_execution_graph interpretation_graph;
    auto interpretation = begin_genesis_performance_trace(
        interpretation_graph,
        "interpretation-boundary.vgm",
        to_flags(provenance_flag::runtime_capture));
    append_genesis_performance_event(
        interpretation_graph,
        interpretation,
        command_event{command_event_kind::command, 100, 0x300, 0x52, ch3_high, 2});
    append_genesis_performance_event(
        interpretation_graph,
        interpretation,
        command_event{command_event_kind::command, 100, 0x303, 0x52, ch3_low, 2});
    const auto ch3_plain_on = append_genesis_performance_event(
        interpretation_graph,
        interpretation,
        command_event{command_event_kind::command, 101, 0x306, 0x52, ch3_key_on, 2});
    CHECK(ch3_plain_on.performance_event_id.has_value());
    CHECK(ch3_plain_on.physical_voice_episode_id.has_value());
    const node_id ch3_episode_id = *ch3_plain_on.physical_voice_episode_id;
    const auto ch3_mode_change = append_genesis_performance_event(
        interpretation_graph,
        interpretation,
        command_event{command_event_kind::command, 102, 0x309, 0x52, ch3_special, 2});
    CHECK(ch3_mode_change.execution.device_transition_id.has_value());
    CHECK(!ch3_mode_change.performance_event_id.has_value());
    CHECK(!interpretation.ym_physical_voice_episode[0][2].has_value());
    const node* ch3_episode = interpretation_graph.find_node(ch3_episode_id);
    CHECK(ch3_episode != nullptr);
    CHECK(ch3_episode->active->end.has_value());
    CHECK(ch3_episode->active->end->tick == 102);
    CHECK(std::get<std::string>(ch3_episode->attributes[5].value) == "simple_pitched_interpretation_lost");
    CHECK(interpretation_graph.nodes_of_kind(node_kind::musical_event).size() == 1);

    // An unreplayable supported command creates an observation gap. The physical
    // episode closes at that boundary and later semantic continuation is blocked
    // until an exact resynchronization, again without a fake release.
    musical_execution_graph gap_graph;
    auto gap = begin_genesis_performance_trace(
        gap_graph,
        "gap.vgm",
        to_flags(provenance_flag::runtime_capture));
    append_genesis_performance_event(
        gap_graph,
        gap,
        command_event{command_event_kind::command, 110, 0x400, 0x52, fnum_high, 2});
    append_genesis_performance_event(
        gap_graph,
        gap,
        command_event{command_event_kind::command, 110, 0x403, 0x52, fnum_low, 2});
    const auto gap_on = append_genesis_performance_event(
        gap_graph,
        gap,
        command_event{command_event_kind::command, 111, 0x406, 0x52, full_key_on, 2});
    CHECK(gap_on.performance_event_id.has_value());
    CHECK(gap_on.physical_voice_episode_id.has_value());
    const node_id gap_episode_id = *gap_on.physical_voice_episode_id;
    const std::uint8_t incomplete_supported[] = {0xA0, 0x77, 0x99};
    const auto gap_event = append_genesis_performance_event(
        gap_graph,
        gap,
        command_event{command_event_kind::command, 112, 0x409, 0x52, incomplete_supported, 3});
    CHECK(!gap_event.execution.device_transition_id.has_value());
    CHECK(!gap_event.performance_event_id.has_value());
    CHECK(!gap.execution.shadow_continuation_valid);
    CHECK(!gap.ym_physical_voice_episode[0][0].has_value());
    const node* gap_episode = gap_graph.find_node(gap_episode_id);
    CHECK(gap_episode != nullptr);
    CHECK(gap_episode->active->end.has_value());
    CHECK(gap_episode->active->end->tick == 112);
    CHECK(std::get<std::string>(gap_episode->attributes[5].value) == "semantic_continuation_lost");
    CHECK(gap_graph.nodes_of_kind(node_kind::musical_event).size() == 1);

    return 0;
}
