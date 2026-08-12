#include "../../components/spc/spc_runtime_voice_adapter.h"

#include <array>
#include <cstdint>
#include <cstring>
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
    return bytes;
}

static spc_voice_runtime_event event(
    spc_voice_runtime_event_kind kind,
    std::uint8_t voice,
    std::int64_t tick,
    std::uint8_t source_index,
    std::uint16_t brr_address) {
    spc_voice_runtime_event value;
    value.kind = kind;
    value.voice = voice;
    value.tick = tick;
    value.tick_rate = 1024000;
    value.source_index = source_index;
    value.brr_address = brr_address;
    return value;
}

int main() {
    const auto snapshot = parse_spc_snapshot(make_fixture());

    musical_execution_graph graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        graph,
        snapshot,
        "runtime-source-fixture.spc",
        to_flags(provenance_flag::runtime_capture));
    auto runtime = begin_spc_runtime_voice_trace(
        graph,
        snapshot_graph,
        "runtime-source-fixture.spc",
        to_flags(provenance_flag::none));

    auto key_on = event(
        spc_voice_runtime_event_kind::key_on_accepted,
        2,
        100,
        3,
        0x3000);
    key_on.key_on_delay = 19;
    const auto onset = append_spc_runtime_voice_event(graph, runtime, key_on);
    CHECK(onset.physical_voice_episode_id.has_value());
    const node_id episode_id = *onset.physical_voice_episode_id;

    auto phase = event(
        spc_voice_runtime_event_kind::sample_phase_started,
        2,
        119,
        3,
        0x3000);
    phase.key_on_delay = 0;
    const auto started = append_spc_runtime_voice_event(graph, runtime, phase);
    CHECK(started.physical_voice_episode_id.has_value());
    CHECK(*started.physical_voice_episode_id == episode_id);

    // SNESAPU keeps mSrc as live per-voice state and may update it independently
    // of physical-voice lifetime. A source change is therefore an observation
    // inside one resource episode, not automatic evidence for a new voice/note.
    const auto source_change = append_spc_runtime_voice_event(
        graph,
        runtime,
        event(
            spc_voice_runtime_event_kind::source_latched,
            2,
            150,
            4,
            0x3400));

    CHECK(source_change.physical_voice_episode_id.has_value());
    CHECK(*source_change.physical_voice_episode_id == episode_id);
    CHECK(runtime.physical_voice_episode[2].has_value());
    CHECK(*runtime.physical_voice_episode[2] == episode_id);
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).size() == 1);

    const node* source_event = graph.find_node(source_change.trace_event_id);
    CHECK(source_event != nullptr);
    CHECK(source_event->kind == node_kind::trace_event);
    CHECK(std::get<std::string>(find_attribute(source_event, "event_kind")->value) == "source_latched");
    CHECK(std::get<std::uint64_t>(find_attribute(source_event, "physical_voice")->value) == 2);
    CHECK(std::get<std::uint64_t>(find_attribute(source_event, "source_index")->value) == 4);
    CHECK(std::get<std::uint64_t>(find_attribute(source_event, "brr_address")->value) == 0x3400);

    const auto contributions = graph.edges_from(source_change.trace_event_id, edge_kind::contributes_to);
    CHECK(contributions.size() == 1);
    CHECK(contributions[0]->to == episode_id);

    // Do not bind this runtime observation to the snapshot BRR object merely
    // because an address happens to match. APURAM is mutable during execution.
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).empty());
    CHECK(graph.edges_from(episode_id, edge_kind::references).empty());

    // A later source change remains in the same physical episode too.
    const auto second_source_change = append_spc_runtime_voice_event(
        graph,
        runtime,
        event(
            spc_voice_runtime_event_kind::source_latched,
            2,
            180,
            5,
            0x3800));
    CHECK(second_source_change.physical_voice_episode_id.has_value());
    CHECK(*second_source_change.physical_voice_episode_id == episode_id);
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).size() == 1);

    const auto ended = append_spc_runtime_voice_event(
        graph,
        runtime,
        event(
            spc_voice_runtime_event_kind::became_inactive,
            2,
            220,
            5,
            0x3800));
    CHECK(ended.physical_voice_episode_id.has_value());
    CHECK(*ended.physical_voice_episode_id == episode_id);
    CHECK(graph.find_node(episode_id)->active->end.has_value());
    CHECK(graph.find_node(episode_id)->active->end->tick == 220);

    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::instrument_definition).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());

    return 0;
}
