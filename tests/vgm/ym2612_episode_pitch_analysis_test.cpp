#include "components/vgm/enhancement/genesis_fm_semantic_adapter.h"
#include "components/vgm/enhancement/ym2612_episode_pitch_analysis.h"

#include <cmath>
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
    std::uint32_t offset,
    std::uint8_t reg,
    std::uint8_t data) {
    const std::uint8_t payload[] = {reg, data};
    return append_genesis_fm_semantic_event(
        graph,
        handle,
        command_event{command_event_kind::command, tick, offset, 0x52, payload, 2});
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

    for (const std::uint8_t reg : {0x30u, 0x34u, 0x38u, 0x3cu})
        ym_write(graph, handle, 0, 0x100 + reg, reg, 0x01);
    ym_write(graph, handle, 0, 0x200, 0xb0, 0x00);
    ym_write(graph, handle, 0, 0x203, 0xa4, 0x2c);
    ym_write(graph, handle, 0, 0x206, 0xa0, 0x00);

    const auto onset = ym_write(graph, handle, 10, 0x209, 0x28, 0xf0);
    CHECK(onset.pitch.performance.physical_voice_episode_id.has_value());
    CHECK(onset.ym2612_synthesis_snapshot_id.has_value());

    genesis_pitch_clock_context clocks;
    clocks.ym2612_clock_hz = 7670454;
    clocks.sn76489_clock_hz = 3579545;
    clocks.source = "fixture-vgm-header";

    const auto features = extract_ym2612_episode_pitch_features(
        graph,
        *onset.pitch.performance.physical_voice_episode_id,
        clocks);
    const auto* periodicity = features.find("fm_network_periodicity_frequency_hz");
    const auto* performed = features.find("performed_pitch_frequency_hz");
    CHECK(periodicity != nullptr);
    CHECK(performed != nullptr);
    CHECK(periodicity->availability == feature_availability::present);
    CHECK(performed->availability == feature_availability::present);
    CHECK(periodicity->claim_layer == semantic_layer::synthesis);
    CHECK(performed->claim_layer == semantic_layer::musical_performance);
    CHECK(periodicity->support_nodes.size() == 2);
    CHECK(periodicity->support_edges.size() == 1);
    CHECK(performed->support_nodes.size() == 2);
    CHECK(performed->status.has_value());
    CHECK(*performed->status == evidence_status::hypothesis);
    CHECK(performed->confidence.has_value());
    CHECK(close_enough(*performed->confidence, ym2612_direct_periodicity_pitch_confidence));

    const auto expected = ym2612_nominal_pitch_frequency_hz(
        0x400,
        5,
        clocks.ym2612_clock_hz);
    CHECK(expected.has_value());
    const auto* performed_hz = std::get_if<double>(&*performed->value);
    CHECK(performed_hz != nullptr);
    CHECK(close_enough(*performed_hz, *expected));

    // Analysis remains graph-only: mutate the live reconstruction after onset.
    // The historical episode still returns the onset pitch from its snapshot.
    ym_write(graph, handle, 20, 0x20c, 0xa4, 0x34);
    ym_write(graph, handle, 20, 0x20f, 0xa0, 0x00);
    const auto frozen_features = extract_ym2612_episode_pitch_features(
        graph,
        *onset.pitch.performance.physical_voice_episode_id,
        clocks);
    const auto* frozen = frozen_features.find("performed_pitch_frequency_hz");
    CHECK(frozen != nullptr);
    CHECK(frozen->availability == feature_availability::present);
    const auto* frozen_hz = std::get_if<double>(&*frozen->value);
    CHECK(frozen_hz != nullptr);
    CHECK(close_enough(*frozen_hz, *expected));

    return 0;
}
