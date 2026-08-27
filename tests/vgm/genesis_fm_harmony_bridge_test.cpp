#include "components/vgm/enhancement/genesis_fm_analysis_features.h"
#include "components/vgm/enhancement/genesis_fm_semantic_adapter.h"
#include "model/analysis_pitch_bridge.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

genesis_fm_semantic_append_result ym_write(
    musical_execution_graph& graph,
    genesis_fm_semantic_graph_handle& handle,
    std::uint64_t tick,
    std::uint32_t offset,
    std::uint8_t reg,
    std::uint8_t data) {
    const std::uint8_t payload[] = {reg, data};
    return append_genesis_fm_semantic_event(
        graph,
        handle,
        command_event{command_event_kind::command, tick, offset, 0x52, payload, 2});
}

node_id add_strong_part(
    musical_execution_graph& graph,
    node_id episode_id,
    const char* label,
    double confidence) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = label;
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    const node_id part_id = graph.add_node(std::move(part));

    edge grouping;
    grouping.kind = edge_kind::groups_into;
    grouping.from = episode_id;
    grouping.to = part_id;
    grouping.provenance.push_back({
        evidence_status::hypothesis,
        confidence,
        "fixture-part-evidence",
        std::nullopt,
        "strong persistent-part fixture for cross-layer integration test",
    });
    graph.add_edge(std::move(grouping));
    return part_id;
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) < tolerance;
}

} // namespace

int main() {
    musical_execution_graph graph;
    auto handle = begin_genesis_fm_semantic_trace(
        graph,
        "fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    // Two ordinary FM channels, each with a clean 1x algorithm-0 operator
    // network. Their FNUM values differ, but no symbolic note representation is
    // introduced anywhere in the path.
    const std::uint8_t op_slots[] = {0x30, 0x34, 0x38, 0x3c};
    for (std::uint8_t reg : op_slots) {
        ym_write(graph, handle, 0, 0x100 + reg, reg, 0x01);
        ym_write(graph, handle, 0, 0x200 + reg, static_cast<std::uint8_t>(reg + 1), 0x01);
    }
    ym_write(graph, handle, 0, 0x300, 0xb0, 0x00);
    ym_write(graph, handle, 0, 0x303, 0xb1, 0x00);
    ym_write(graph, handle, 0, 0x306, 0xa4, 0x2c); // ch0: FNUM 0x400, block 5
    ym_write(graph, handle, 0, 0x309, 0xa0, 0x00);
    ym_write(graph, handle, 0, 0x30c, 0xa5, 0x2d); // ch1: FNUM 0x500, block 5
    ym_write(graph, handle, 0, 0x30f, 0xa1, 0x00);

    const auto first_onset = ym_write(graph, handle, 10, 0x312, 0x28, 0xf0);
    const auto second_onset = ym_write(graph, handle, 10, 0x315, 0x28, 0xf1);
    CHECK(first_onset.pitch.performance.performance_event_id.has_value());
    CHECK(second_onset.pitch.performance.performance_event_id.has_value());
    CHECK(first_onset.pitch.performance.physical_voice_episode_id.has_value());
    CHECK(second_onset.pitch.performance.physical_voice_episode_id.has_value());

    const node_id first_part = add_strong_part(
        graph,
        *first_onset.pitch.performance.physical_voice_episode_id,
        "persistent part A",
        0.91);
    const node_id second_part = add_strong_part(
        graph,
        *second_onset.pitch.performance.physical_voice_episode_id,
        "persistent part B",
        0.89);

    genesis_pitch_clock_context clocks;
    clocks.ym2612_clock_hz = 7670454;
    clocks.sn76489_clock_hz = 3579545;
    clocks.source = "fixture-vgm-header";

    const auto first_features = extract_genesis_fm_part_aware_performance_analysis_features(
        graph,
        *first_onset.pitch.performance.performance_event_id,
        clocks);
    const auto second_features = extract_genesis_fm_part_aware_performance_analysis_features(
        graph,
        *second_onset.pitch.performance.performance_event_id,
        clocks);

    const auto first_pitch = absolute_musical_pitch_from_analysis_features(
        graph,
        *first_onset.pitch.performance.performance_event_id,
        first_features,
        "performed_pitch_frequency_hz",
        musical_pitch_role::performed,
        "genesis-fm-harmony-bridge");
    const auto second_pitch = absolute_musical_pitch_from_analysis_features(
        graph,
        *second_onset.pitch.performance.performance_event_id,
        second_features,
        "performed_pitch_frequency_hz",
        musical_pitch_role::performed,
        "genesis-fm-harmony-bridge");
    CHECK(first_pitch.has_value());
    CHECK(second_pitch.has_value());
    CHECK(first_pitch->part_id == first_part);
    CHECK(second_pitch->part_id == second_part);
    CHECK(first_pitch->role == musical_pitch_role::performed);
    CHECK(second_pitch->role == musical_pitch_role::performed);

    const auto verticality = make_harmonic_verticality(
        time_coordinate{time_domain::source, 10, 0, 0},
        {*first_pitch, *second_pitch});
    CHECK(verticality.part_ids.size() == 2);
    CHECK(verticality.frequencies_hz.size() == 2);
    CHECK(verticality.intervals_above_lowest_octaves.size() == 2);
    CHECK(verticality.role == musical_pitch_role::performed);
    CHECK(close_enough(verticality.confidence, ym2612_direct_periodicity_pitch_confidence));

    // FNUM 0x500 / 0x400 = 1.25 at the same block and multiplier lattice.
    CHECK(close_enough(
        verticality.intervals_above_lowest_octaves[1],
        std::log2(1.25)));

    return 0;
}
