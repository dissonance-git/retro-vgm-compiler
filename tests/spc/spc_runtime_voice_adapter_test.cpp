#include "../../components/spc/spc_runtime_voice_adapter.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static const attribute* find_attribute(const node* value, const char* key) {
    if (value == nullptr)
        return nullptr;
    for (const auto& item : value->attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

static std::array<std::uint8_t, spc_full_file_size> make_fixture() {
    std::array<std::uint8_t, spc_full_file_size> bytes{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data v0.30\x1A\x1A";
    static_assert(sizeof(signature) - 1 == spc_signature_size, "unexpected SPC signature size");
    std::memcpy(bytes.data(), signature, spc_signature_size);
    bytes[0x24] = 30;

    // Deliberately make the saved register image look active. A static SPC
    // snapshot must still not become a runtime voice episode by itself.
    bytes[spc_dsp_offset + 0x04] = 0x03;
    bytes[spc_dsp_offset + 0x08] = 0x40;
    bytes[spc_dsp_offset + 0x09] = 0x20;
    bytes[spc_dsp_offset + 0x4C] = 0x01;
    bytes[spc_dsp_offset + 0x5D] = 0x20;

    const std::size_t directory_entry = spc_ram_offset + 0x200C;
    bytes[directory_entry + 0] = 0x00;
    bytes[directory_entry + 1] = 0x30;
    bytes[directory_entry + 2] = 0x00;
    bytes[directory_entry + 3] = 0x30;
    bytes[spc_ram_offset + 0x3000] = 0x01;
    return bytes;
}

static spc_voice_runtime_event voice_event(
    spc_voice_runtime_event_kind kind,
    std::uint8_t voice,
    std::int64_t tick) {
    spc_voice_runtime_event event;
    event.kind = kind;
    event.voice = voice;
    event.tick = tick;
    event.tick_rate = 1024000;
    return event;
}

int main() {
    const auto bytes = make_fixture();
    const auto snapshot = parse_spc_snapshot(bytes);

    musical_execution_graph graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        graph,
        snapshot,
        "runtime-fixture.spc",
        to_flags(provenance_flag::runtime_capture));

    CHECK(graph.nodes_of_kind(node_kind::voice_instance).empty());

    auto runtime = begin_spc_runtime_voice_trace(
        graph,
        snapshot_graph,
        "runtime-fixture.spc",
        to_flags(provenance_flag::none));

    const node* runtime_trace = graph.find_node(runtime.execution_trace_id);
    CHECK(runtime_trace != nullptr);
    CHECK(runtime_trace->kind == node_kind::execution_trace);
    CHECK(runtime_trace->layer == semantic_layer::synthesis);
    CHECK(has_flag(runtime_trace->provenance[0].flags, provenance_flag::runtime_capture));

    const auto snapshot_derivations = graph.edges_from(snapshot_graph.snapshot_id, edge_kind::derived_from);
    CHECK(snapshot_derivations.size() == 1);
    CHECK(snapshot_derivations[0]->to == runtime.execution_trace_id);

    // Release state without an observed key-on never manufactures an episode.
    const auto orphan_release = append_spc_runtime_voice_event(
        graph,
        runtime,
        voice_event(spc_voice_runtime_event_kind::release_entered, 0, 90));
    CHECK(!orphan_release.physical_voice_episode_id.has_value());
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).empty());

    auto key_on = voice_event(spc_voice_runtime_event_kind::key_on_accepted, 0, 100);
    key_on.source_index = 3;
    key_on.brr_address = 0x3000;
    key_on.envelope_value = 0;
    key_on.pitch_rate = 0x12340;
    key_on.key_on_delay = 19;
    key_on.noise_enabled = false;
    const auto onset = append_spc_runtime_voice_event(graph, runtime, key_on);
    CHECK(onset.physical_voice_episode_id.has_value());
    const node_id first_episode_id = *onset.physical_voice_episode_id;
    CHECK(runtime.physical_voice_episode[0].has_value());
    CHECK(*runtime.physical_voice_episode[0] == first_episode_id);

    const node* first_episode = graph.find_node(first_episode_id);
    CHECK(first_episode != nullptr);
    CHECK(first_episode->kind == node_kind::voice_instance);
    CHECK(first_episode->layer == semantic_layer::synthesis);
    CHECK(first_episode->active.has_value());
    CHECK(first_episode->active->start.domain == time_domain::device);
    CHECK(first_episode->active->start.tick == 100);
    CHECK(first_episode->active->start.tick_rate == 1024000);
    CHECK(!first_episode->active->end.has_value());
    CHECK(std::get<std::string>(find_attribute(first_episode, "episode_origin")->value) == "observed_key_on_acceptance");
    CHECK(std::get<std::uint64_t>(find_attribute(first_episode, "initial_source_index")->value) == 3);
    CHECK(std::get<std::uint64_t>(find_attribute(first_episode, "initial_key_on_delay")->value) == 19);
    CHECK(std::get<std::string>(find_attribute(first_episode, "persistent_part_identity")->value) == "unresolved");

    const auto occupancy = graph.edges_from(first_episode_id, edge_kind::occupies);
    CHECK(occupancy.size() == 1);
    CHECK(occupancy[0]->to == snapshot_graph.voice_slot_ids[0]);

    const auto onset_causes = graph.edges_from(onset.trace_event_id, edge_kind::causes);
    CHECK(onset_causes.size() == 1);
    CHECK(onset_causes[0]->to == first_episode_id);

    // Expiry of the KON delay changes the runtime phase of the same physical
    // episode. It is not a second note onset and it does not create a new voice.
    auto sample_phase = voice_event(spc_voice_runtime_event_kind::sample_phase_started, 0, 119);
    sample_phase.source_index = 3;
    sample_phase.brr_address = 0x3000;
    sample_phase.key_on_delay = 0;
    const auto sample_started = append_spc_runtime_voice_event(graph, runtime, sample_phase);
    CHECK(sample_started.physical_voice_episode_id.has_value());
    CHECK(*sample_started.physical_voice_episode_id == first_episode_id);
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).size() == 1);
    CHECK(graph.edges_from(sample_started.trace_event_id, edge_kind::contributes_to).size() == 1);

    // KOFF/release does not terminate the voice. The envelope may continue to
    // produce sound until the DSP reports the resource inactive.
    auto release = voice_event(spc_voice_runtime_event_kind::release_entered, 0, 200);
    release.envelope_value = 0x500;
    const auto released = append_spc_runtime_voice_event(graph, runtime, release);
    CHECK(released.physical_voice_episode_id.has_value());
    CHECK(*released.physical_voice_episode_id == first_episode_id);
    CHECK(!graph.find_node(first_episode_id)->active->end.has_value());

    auto inactive = voice_event(spc_voice_runtime_event_kind::became_inactive, 0, 250);
    inactive.envelope_value = 0;
    const auto ended = append_spc_runtime_voice_event(graph, runtime, inactive);
    CHECK(ended.physical_voice_episode_id.has_value());
    CHECK(*ended.physical_voice_episode_id == first_episode_id);
    CHECK(!runtime.physical_voice_episode[0].has_value());
    CHECK(graph.find_node(first_episode_id)->active->end.has_value());
    CHECK(graph.find_node(first_episode_id)->active->end->tick == 250);
    CHECK(std::get<std::string>(find_attribute(graph.find_node(first_episode_id), "termination_reason")->value) == "became_inactive");
    CHECK(std::get<bool>(find_attribute(graph.find_node(first_episode_id), "termination_boundary_complete")->value));

    // A new accepted KON is a new bounded physical episode even on the same
    // hardware voice. Retriggering before inactivity closes the previous one.
    auto second_key_on = voice_event(spc_voice_runtime_event_kind::key_on_accepted, 0, 300);
    second_key_on.source_index = 4;
    second_key_on.key_on_delay = 19;
    const auto second_onset = append_spc_runtime_voice_event(graph, runtime, second_key_on);
    CHECK(second_onset.physical_voice_episode_id.has_value());
    const node_id second_episode_id = *second_onset.physical_voice_episode_id;
    CHECK(second_episode_id != first_episode_id);

    auto retrigger = voice_event(spc_voice_runtime_event_kind::key_on_accepted, 0, 320);
    retrigger.source_index = 5;
    retrigger.key_on_delay = 19;
    const auto retriggered = append_spc_runtime_voice_event(graph, runtime, retrigger);
    CHECK(retriggered.physical_voice_episode_id.has_value());
    const node_id third_episode_id = *retriggered.physical_voice_episode_id;
    CHECK(third_episode_id != second_episode_id);
    CHECK(graph.find_node(second_episode_id)->active->end.has_value());
    CHECK(graph.find_node(second_episode_id)->active->end->tick == 320);
    CHECK(std::get<std::string>(find_attribute(graph.find_node(second_episode_id), "termination_reason")->value) == "retriggered_by_key_on");

    // A capture/semantic gap ends continuity but does not fabricate release or
    // inactivity. The boundary is explicitly marked incomplete.
    spc_voice_runtime_event gap;
    gap.kind = spc_voice_runtime_event_kind::continuation_lost;
    gap.tick = 330;
    gap.tick_rate = 1024000;
    const auto lost = append_spc_runtime_voice_event(graph, runtime, gap);
    CHECK(!lost.physical_voice_episode_id.has_value());
    CHECK(!runtime.physical_voice_episode[0].has_value());
    CHECK(graph.find_node(third_episode_id)->active->end.has_value());
    CHECK(graph.find_node(third_episode_id)->active->end->tick == 330);
    CHECK(std::get<std::string>(find_attribute(graph.find_node(third_episode_id), "termination_reason")->value) == "semantic_continuation_lost");
    CHECK(!std::get<bool>(find_attribute(graph.find_node(third_episode_id), "termination_boundary_complete")->value));
    CHECK(has_flag(graph.find_node(lost.trace_event_id)->provenance[0].flags, provenance_flag::incomplete));

    // Later phase observations cannot bridge that gap by inventing an onset.
    const auto post_gap_phase = append_spc_runtime_voice_event(
        graph,
        runtime,
        voice_event(spc_voice_runtime_event_kind::sample_phase_started, 0, 340));
    CHECK(!post_gap_phase.physical_voice_episode_id.has_value());

    // Reset also terminates an observed episode, but with a complete explicit
    // boundary rather than a gap.
    const auto fourth_onset = append_spc_runtime_voice_event(
        graph,
        runtime,
        voice_event(spc_voice_runtime_event_kind::key_on_accepted, 1, 400));
    CHECK(fourth_onset.physical_voice_episode_id.has_value());
    const node_id fourth_episode_id = *fourth_onset.physical_voice_episode_id;

    spc_voice_runtime_event reset;
    reset.kind = spc_voice_runtime_event_kind::execution_reset;
    reset.tick = 410;
    reset.tick_rate = 1024000;
    append_spc_runtime_voice_event(graph, runtime, reset);
    CHECK(graph.find_node(fourth_episode_id)->active->end.has_value());
    CHECK(graph.find_node(fourth_episode_id)->active->end->tick == 410);
    CHECK(std::get<std::string>(find_attribute(graph.find_node(fourth_episode_id), "termination_reason")->value) == "execution_reset");
    CHECK(std::get<bool>(find_attribute(graph.find_node(fourth_episode_id), "termination_boundary_complete")->value));

    bool rejected_invalid_voice = false;
    try {
        append_spc_runtime_voice_event(
            graph,
            runtime,
            voice_event(spc_voice_runtime_event_kind::key_on_accepted, 8, 500));
    } catch (const std::invalid_argument&) {
        rejected_invalid_voice = true;
    }
    CHECK(rejected_invalid_voice);

    // Runtime synthesis evidence still does not automatically become notes,
    // persistent musical parts, instruments, or notation/MIDI projections.
    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::instrument_definition).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());

    return 0;
}
