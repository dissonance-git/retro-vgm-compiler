#include "components/vgm/enhancement/genesis_analysis_features.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

namespace {

const analysis_feature* require_feature(
    const analysis_feature_set& features,
    const char* name) {
    const analysis_feature* feature = features.find(name);
    assert(feature != nullptr);
    return feature;
}

bool near(double lhs, double rhs, double tolerance) {
    return std::abs(lhs - rhs) <= tolerance;
}

} // namespace

int main() {
    musical_execution_graph graph;
    auto trace = begin_genesis_performance_trace(
        graph,
        "sonic-clock-fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    // YM2612 FNUM 0x434, BLOCK 5.
    const std::uint8_t fnum_high[] = {0xA4, 0x2C};
    const std::uint8_t fnum_low[] = {0xA0, 0x34};
    append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 10, 0x100, 0x52, fnum_high, 2});
    append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 10, 0x103, 0x52, fnum_low, 2});

    const std::uint8_t full_key_on[] = {0x28, 0xF0};
    const auto ym_on = append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 20, 0x106, 0x52, full_key_on, 2});
    assert(ym_on.performance_event_id.has_value());

    // Without source clock context the common-frequency question stays unknown.
    const auto ym_without_clocks = extract_genesis_performance_analysis_features(
        graph,
        *ym_on.performance_event_id);
    assert(require_feature(ym_without_clocks, "device_nominal_pitch_frequency_hz")->availability ==
           feature_availability::unknown);
    assert(require_feature(ym_without_clocks, "performed_pitch_frequency_hz")->availability ==
           feature_availability::unknown);

    const genesis_pitch_clock_context sonic_clocks{
        7670453,
        3579545,
        "fixture VGM header clocks",
    };
    const auto ym_with_clocks = extract_genesis_performance_analysis_features(
        graph,
        *ym_on.performance_event_id,
        &sonic_clocks);

    const analysis_feature* ym_nominal =
        require_feature(ym_with_clocks, "device_nominal_pitch_frequency_hz");
    assert(ym_nominal->availability == feature_availability::present);
    assert(ym_nominal->status == evidence_status::derived);
    assert(ym_nominal->claim_layer == semantic_layer::synthesis);
    assert(ym_nominal->unit == "Hz");
    assert(near(std::get<double>(ym_nominal->value.value()), 874.5625207689073, 1e-9));
    assert(!ym_nominal->provenance.empty());

    // The important boundary: normalizing FNUM/BLOCK to Hz does not claim the
    // FM patch's performed or perceived pitch. Operator ratios, detune, PM/LFO,
    // acoustic output and auditory evidence remain downstream questions.
    const analysis_feature* ym_performed =
        require_feature(ym_with_clocks, "performed_pitch_frequency_hz");
    assert(ym_performed->availability == feature_availability::unknown);
    assert(!ym_performed->value.has_value());
    assert(ym_performed->claim_layer == semantic_layer::musical_performance);

    // PSG uses a different native coordinate but lands in the same nominal-Hz
    // feature when its source clock is supplied.
    const std::uint8_t psg_tone_latch = 0x85;
    const std::uint8_t psg_tone_data = 0x12;
    append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 70, 0x130, 0x50, &psg_tone_latch, 1});
    append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 70, 0x132, 0x50, &psg_tone_data, 1});
    const std::uint8_t psg_unmute = 0x90;
    const auto psg_on = append_genesis_performance_event(
        graph,
        trace,
        command_event{command_event_kind::command, 71, 0x134, 0x50, &psg_unmute, 1});
    assert(psg_on.performance_event_id.has_value());

    const auto psg_with_clocks = extract_genesis_performance_analysis_features(
        graph,
        *psg_on.performance_event_id,
        &sonic_clocks);
    const analysis_feature* psg_nominal =
        require_feature(psg_with_clocks, "device_nominal_pitch_frequency_hz");
    assert(psg_nominal->availability == feature_availability::present);
    assert(psg_nominal->claim_layer == semantic_layer::synthesis);
    assert(near(std::get<double>(psg_nominal->value.value()), 381.7774104095563, 1e-9));

    // A context with the wrong device clock missing does not manufacture a
    // frequency from the other chip's clock.
    const genesis_pitch_clock_context ym_only{
        7670453,
        0,
        "YM-only clock fixture",
    };
    const auto psg_missing_clock = extract_genesis_performance_analysis_features(
        graph,
        *psg_on.performance_event_id,
        &ym_only);
    assert(require_feature(psg_missing_clock, "device_nominal_pitch_frequency_hz")->availability ==
           feature_availability::unknown);

    return 0;
}
