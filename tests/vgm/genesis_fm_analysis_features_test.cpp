#include "components/vgm/enhancement/genesis_fm_analysis_features.h"
#include "components/vgm/enhancement/genesis_fm_semantic_adapter.h"

#include <cmath>
#include <cstdint>

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

    // Every operator runs at 2x. The channel FNUM still describes the nominal
    // device basis, while the static FM network itself repeats at 2x.
    const std::uint8_t operator_regs[] = {0x30, 0x34, 0x38, 0x3c};
    for (std::size_t index = 0; index < 4; ++index)
        ym_write(graph, handle, 0, 0x100 + index * 3, operator_regs[index], 0x02);
    ym_write(graph, handle, 0, 0x120, 0xb0, 0x00);
    ym_write(graph, handle, 0, 0x123, 0xa4, 0x2c);
    ym_write(graph, handle, 0, 0x126, 0xa0, 0x00);
    const auto onset = ym_write(graph, handle, 10, 0x129, 0x28, 0xf0);
    CHECK(onset.pitch.performance.performance_event_id.has_value());

    genesis_pitch_clock_context clocks;
    clocks.ym2612_clock_hz = 7670454;
    clocks.sn76489_clock_hz = 3579545;
    clocks.source = "fixture-vgm-header";

    const auto features = extract_genesis_fm_part_aware_performance_analysis_features(
        graph,
        *onset.pitch.performance.performance_event_id,
        clocks);
    const auto* nominal = features.find("device_nominal_pitch_frequency_hz");
    const auto* periodicity = features.find("fm_network_periodicity_frequency_hz");
    const auto* performed = features.find("performed_pitch_frequency_hz");
    CHECK(nominal != nullptr && periodicity != nullptr && performed != nullptr);
    CHECK(nominal->availability == feature_availability::present);
    CHECK(periodicity->availability == feature_availability::present);
    CHECK(performed->availability == feature_availability::present);
    CHECK(nominal->claim_layer == semantic_layer::synthesis);
    CHECK(periodicity->claim_layer == semantic_layer::synthesis);
    CHECK(performed->claim_layer == semantic_layer::musical_performance);

    const auto* nominal_hz = std::get_if<double>(&*nominal->value);
    const auto* periodicity_hz = std::get_if<double>(&*periodicity->value);
    const auto* performed_hz = std::get_if<double>(&*performed->value);
    CHECK(nominal_hz != nullptr && periodicity_hz != nullptr && performed_hz != nullptr);
    CHECK(close_enough(*periodicity_hz, *nominal_hz * 2.0));
    CHECK(close_enough(*performed_hz, *periodicity_hz));
    CHECK(performed->status.has_value());
    CHECK(*performed->status == evidence_status::hypothesis);
    CHECK(performed->confidence.has_value());
    CHECK(close_enough(*performed->confidence, ym2612_direct_periodicity_pitch_confidence));

    // Part identity remains a separate question. FM pitch understanding does not
    // manufacture a persistent musical part just because performed pitch became available.
    const auto* part = features.find("persistent_part_identity");
    CHECK(part != nullptr);
    CHECK(part->availability == feature_availability::unknown);

    return 0;
}
