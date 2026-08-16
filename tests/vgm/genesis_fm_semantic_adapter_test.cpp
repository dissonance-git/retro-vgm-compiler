#include "components/vgm/enhancement/genesis_fm_semantic_adapter.h"

#include <cstdint>
#include <string>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

genesis_fm_semantic_append_result ym_write(
    musical_execution_graph& graph,
    genesis_fm_semantic_graph_handle& handle,
    std::uint64_t tick,
    std::uint64_t offset,
    std::uint8_t reg,
    std::uint8_t data) {
    const std::uint8_t payload[] = {reg, data};
    return append_genesis_fm_semantic_event(
        graph,
        handle,
        command_event{command_event_kind::command, tick, offset, 0x52, payload, 2});
}

const attribute* find_attr(const node& value, const char* key) {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

std::uint64_t uint_attr(const node& value, const char* key) {
    const auto* item = find_attr(value, key);
    if (item == nullptr)
        return 0xffffffffffffffffull;
    const auto* number = std::get_if<std::uint64_t>(&item->value);
    return number == nullptr ? 0xffffffffffffffffull : *number;
}

} // namespace

int main() {
    musical_execution_graph graph;
    auto handle = begin_genesis_fm_semantic_trace(
        graph,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    // Establish a simple algorithm-0 patch and a channel pitch before key-on.
    // Yamaha register slots are OP1, OP3, OP2, OP4; genesis_state maps them to
    // logical OP1..OP4 internally.
    ym_write(graph, handle, 0, 0x100, 0x30, 0x01);
    ym_write(graph, handle, 0, 0x103, 0x34, 0x01);
    ym_write(graph, handle, 0, 0x106, 0x38, 0x01);
    ym_write(graph, handle, 0, 0x109, 0x3c, 0x01);
    ym_write(graph, handle, 0, 0x10c, 0xb0, 0x00);
    ym_write(graph, handle, 0, 0x10f, 0xa4, 0x2c);
    ym_write(graph, handle, 0, 0x112, 0xa0, 0x00);

    const auto onset = ym_write(graph, handle, 10, 0x115, 0x28, 0xf0);
    CHECK(onset.pitch.performance.performance_event_id.has_value());
    CHECK(onset.pitch.performance.physical_voice_episode_id.has_value());
    CHECK(onset.ym2612_synthesis_snapshot_id.has_value());

    const node* snapshot = graph.find_node(*onset.ym2612_synthesis_snapshot_id);
    CHECK(snapshot != nullptr);
    CHECK(snapshot->kind == node_kind::synthesis_object);
    CHECK(snapshot->layer == semantic_layer::synthesis);
    CHECK(snapshot->active.has_value());
    CHECK(snapshot->active->start.tick == 10);
    CHECK(uint_attr(*snapshot, "physical_channel") == 0);
    CHECK(uint_attr(*snapshot, "fnum") == 0x400);
    CHECK(uint_attr(*snapshot, "block") == 5);
    CHECK(uint_attr(*snapshot, "algorithm") == 0);
    CHECK(uint_attr(*snapshot, "operator_key_mask") == 0x0f);
    CHECK(uint_attr(*snapshot, "op1_multiple") == 1);
    CHECK(uint_attr(*snapshot, "op2_multiple") == 1);
    CHECK(uint_attr(*snapshot, "op3_multiple") == 1);
    CHECK(uint_attr(*snapshot, "op4_multiple") == 1);
    CHECK(!snapshot->provenance.empty());
    CHECK(snapshot->provenance.front().byte_offset.has_value());
    CHECK(*snapshot->provenance.front().byte_offset == 0x115);

    const auto snapshot_links = graph.edges_from(
        *onset.ym2612_synthesis_snapshot_id,
        edge_kind::contributes_to);
    CHECK(snapshot_links.size() == 1);
    CHECK(snapshot_links.front()->to == *onset.pitch.performance.physical_voice_episode_id);

    // A later live patch change must not mutate the episode-onset snapshot.
    ym_write(graph, handle, 20, 0x118, 0xb0, 0x07);
    snapshot = graph.find_node(*onset.ym2612_synthesis_snapshot_id);
    CHECK(snapshot != nullptr);
    CHECK(uint_attr(*snapshot, "algorithm") == 0);
    CHECK(handle.pitch.performance.execution.shadow.ym2612().channels[0].algorithm == 7);

    // Release reports the same bounded episode but must not materialize another
    // onset snapshot.
    const auto release = ym_write(graph, handle, 30, 0x11b, 0x28, 0x00);
    CHECK(release.pitch.performance.performance_event_id.has_value());
    CHECK(release.pitch.performance.physical_voice_episode_id.has_value());
    CHECK(!release.ym2612_synthesis_snapshot_id.has_value());

    return 0;
}
