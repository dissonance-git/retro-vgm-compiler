#include "components/vgm/enhancement/ym2612_fundamental_hypothesis.h"

#include <cmath>
#include <cstdint>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

genesis_pitch_clock_context clocks() {
    return {7670454, 3579545, "fixture-vgm-header"};
}

ym2612_state base_state(std::uint8_t algorithm) {
    ym2612_state state;
    auto& channel = state.channels[0];
    channel.fnum = 0x400;
    channel.block = 5;
    channel.algorithm = algorithm;
    channel.operator_key_mask = 0x0f;
    channel.key_on = true;
    for (auto& op : channel.operators) {
        op.multiple = 1;
        op.detune = 0;
        op.total_level = 0;
    }
    return state;
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) < tolerance;
}

} // namespace

int main() {
    const auto clock_context = clocks();
    const auto nominal = ym2612_nominal_pitch_frequency_hz(
        0x400,
        5,
        clock_context.ym2612_clock_hz);
    CHECK(nominal.has_value());

    // Algorithm 0 has only OP4 as a direct carrier. If every operator is 1x,
    // the carrier and whole-network periodicity agree at the channel basis.
    auto alg0 = base_state(0);
    auto simple = infer_ym2612_fundamental_hypothesis(alg0, 0, clock_context);
    CHECK(simple.carrier_mask == 0b1000);
    CHECK(simple.operators.size() == 4);
    CHECK(simple.distinct_carrier_ratios.size() == 1);
    CHECK(close_enough(simple.distinct_carrier_ratios[0], 1.0));
    CHECK(simple.network_periodicity_ratio.has_value());
    CHECK(close_enough(*simple.network_periodicity_ratio, 1.0));
    CHECK(simple.periodicity_has_direct_carrier);
    CHECK(!simple.missing_fundamental_case);
    CHECK(simple.performed_pitch_frequency_hz.has_value());
    CHECK(close_enough(*simple.performed_pitch_frequency_hz, *nominal));
    CHECK(close_enough(simple.confidence, ym2612_direct_periodicity_pitch_confidence));

    // Classic converter trap: OP4 is a 2x direct carrier but its upstream
    // modulators are 1x. The static FM network still repeats at 1x, so treating
    // the carrier alone as the note would be an octave-high projection.
    alg0.channels[0].operators[3].multiple = 2;
    const auto missing = infer_ym2612_fundamental_hypothesis(alg0, 0, clock_context);
    CHECK(missing.distinct_carrier_ratios.size() == 1);
    CHECK(close_enough(missing.distinct_carrier_ratios[0], 2.0));
    CHECK(missing.network_periodicity_ratio.has_value());
    CHECK(close_enough(*missing.network_periodicity_ratio, 1.0));
    CHECK(!missing.periodicity_has_direct_carrier);
    CHECK(missing.missing_fundamental_case);
    CHECK(missing.performed_pitch_frequency_hz.has_value());
    CHECK(close_enough(*missing.performed_pitch_frequency_hz, *nominal));
    CHECK(close_enough(missing.confidence, ym2612_missing_fundamental_pitch_ceiling));

    // OPN MULT register zero means 0.5x, exactly as ymfm's x.1 cache models it.
    auto half = base_state(7);
    half.channels[0].operators[0].multiple = 0;
    CHECK(close_enough(ym2612_operator_multiplier_ratio(0), 0.5));
    const auto half_result = infer_ym2612_fundamental_hypothesis(half, 0, clock_context);
    CHECK(half_result.carrier_mask == 0b1111);
    CHECK(half_result.network_periodicity_ratio.has_value());
    CHECK(close_enough(*half_result.network_periodicity_ratio, 0.5));
    CHECK(half_result.periodicity_has_direct_carrier);
    CHECK(half_result.performed_pitch_frequency_hz.has_value());
    CHECK(close_enough(*half_result.performed_pitch_frequency_hz, *nominal * 0.5));

    // Algorithm 4 has OP2 + OP4 carriers. Mixed direct carrier ratios do not
    // destroy the common network periodicity when all operator ratios remain
    // rational and static.
    auto alg4 = base_state(4);
    alg4.channels[0].operators[1].multiple = 2;
    const auto two_carriers = infer_ym2612_fundamental_hypothesis(alg4, 0, clock_context);
    CHECK(two_carriers.carrier_mask == 0b1010);
    CHECK(two_carriers.distinct_carrier_ratios.size() == 2);
    CHECK(two_carriers.network_periodicity_ratio.has_value());
    CHECK(close_enough(*two_carriers.network_periodicity_ratio, 1.0));
    CHECK(two_carriers.periodicity_has_direct_carrier);
    CHECK(!two_carriers.performed_pitch_ambiguous);

    // Partial operator keying is deliberately conservative because a keyed-off
    // operator may still be in release and this state layer does not track its
    // instantaneous envelope amplitude.
    auto partial = base_state(7);
    partial.channels[0].operator_key_mask = 0x0e;
    const auto partial_result = infer_ym2612_fundamental_hypothesis(partial, 0, clock_context);
    CHECK(!partial_result.all_operators_keyed);
    CHECK(!partial_result.network_periodicity_ratio.has_value());
    CHECK(!partial_result.performed_pitch_frequency_hz.has_value());
    CHECK(partial_result.performed_pitch_ambiguous);
    CHECK(close_enough(partial_result.confidence, ym2612_partially_keyed_periodicity_ceiling));

    // Detune and active phase modulation invalidate the exact static lattice.
    auto detuned = base_state(7);
    detuned.channels[0].operators[2].detune = 1;
    const auto detuned_result = infer_ym2612_fundamental_hypothesis(detuned, 0, clock_context);
    CHECK(detuned_result.detune_present);
    CHECK(!detuned_result.performed_pitch_frequency_hz.has_value());
    CHECK(close_enough(detuned_result.confidence, ym2612_detuned_periodicity_ceiling));

    auto pm = base_state(7);
    pm.lfo_enabled = true;
    pm.channels[0].fms = 4;
    const auto pm_result = infer_ym2612_fundamental_hypothesis(pm, 0, clock_context);
    CHECK(pm_result.phase_modulation_active);
    CHECK(!pm_result.performed_pitch_frequency_hz.has_value());
    CHECK(close_enough(pm_result.confidence, ym2612_pm_periodicity_ceiling));

    // Ordinary channel-level pitch must not leak into CH3 special mode or DAC.
    auto ch3 = base_state(7);
    ch3.channels[2] = ch3.channels[0];
    ch3.channel3_mode = 1;
    const auto ch3_result = infer_ym2612_fundamental_hypothesis(ch3, 2, clock_context);
    CHECK(!ch3_result.channel_pitch_interpretation_supported);
    CHECK(!ch3_result.performed_pitch_frequency_hz.has_value());

    auto dac = base_state(7);
    dac.channels[5] = dac.channels[0];
    dac.dac_enabled = true;
    const auto dac_result = infer_ym2612_fundamental_hypothesis(dac, 5, clock_context);
    CHECK(!dac_result.channel_pitch_interpretation_supported);
    CHECK(!dac_result.performed_pitch_frequency_hz.has_value());

    // Only an unambiguous performed-pitch hypothesis may enter the shared
    // absolute-musical-pitch layer. It still carries hypothesis status.
    const auto performed = ym2612_fundamental_as_performed_pitch(
        simple,
        123,
        77,
        time_span{{time_domain::source, 10, 0, 0}, {time_domain::source, 20, 0, 0}},
        "ym2612-operator-network-analysis");
    CHECK(performed.has_value());
    CHECK(performed->role == musical_pitch_role::performed);
    CHECK(performed->status == evidence_status::hypothesis);
    CHECK(performed->part_id == 77);
    CHECK(close_enough(performed->frequency_hz, *nominal));

    return 0;
}
