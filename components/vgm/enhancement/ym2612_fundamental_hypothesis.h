#pragma once

#include "genesis_nominal_pitch.h"
#include "genesis_state.h"
#include "../../../model/harmonic_verticality.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

struct ym2612_operator_pitch_component {
    std::size_t operator_index = 0;
    std::uint8_t multiple_register = 0;
    double multiplier_ratio = 1.0;
    double nominal_operator_frequency_hz = 0.0;
    std::uint8_t total_level = 0;
    bool keyed = false;
    bool carrier = false;
};

struct ym2612_fundamental_hypothesis {
    std::uint8_t algorithm = 0;
    std::uint8_t carrier_mask = 0;
    std::vector<ym2612_operator_pitch_component> operators;
    std::vector<double> distinct_carrier_ratios;
    std::vector<double> distinct_operator_ratios;

    // Static spectral periodicity under ordinary shared-FNUM OPN synthesis.
    // OPN MULT values are integer or 0.5x, so with detune/PM absent the common
    // repeat rate is the greatest common divisor of all participating ratios
    // expressed in half-units. This is not automatically perceived pitch.
    std::optional<double> network_periodicity_ratio{};
    std::optional<double> network_periodicity_frequency_hz{};
    bool periodicity_has_direct_carrier = false;

    // Promoted only when the static synthesis evidence is strong enough to make
    // one bounded performed-pitch hypothesis. Heard/perceptual pitch remains a
    // separate layer.
    std::optional<double> performed_pitch_frequency_hz{};

    bool carrier_ratios_unanimous = false;
    bool detune_present = false;
    bool phase_modulation_active = false;
    bool all_operators_keyed = false;
    bool all_carriers_keyed = false;
    bool channel_pitch_interpretation_supported = false;
    bool performed_pitch_ambiguous = true;
    bool missing_fundamental_case = false;
    double confidence = 0.0;
    std::string detail;
};

constexpr double ym2612_direct_periodicity_pitch_confidence = 0.86;
constexpr double ym2612_missing_fundamental_pitch_ceiling = 0.68;
constexpr double ym2612_detuned_periodicity_ceiling = 0.60;
constexpr double ym2612_pm_periodicity_ceiling = 0.55;
constexpr double ym2612_partially_keyed_periodicity_ceiling = 0.49;

// Carrier topology follows the OPM/OPN output_4op routing implemented by ymfm:
// 0-3: OP4 only; 4: OP2+OP4; 5-6: OP2+OP3+OP4; 7: all operators.
inline constexpr std::array<std::uint8_t, 8> ym2612_algorithm_carrier_masks = {
    0b1000,
    0b1000,
    0b1000,
    0b1000,
    0b1010,
    0b1110,
    0b1110,
    0b1111,
};

// ymfm caches OPN MULT as an x.1 value: register 0 becomes 1 (0.5x), while
// register values 1..15 become 2..30 (1x..15x).
inline constexpr std::uint8_t ym2612_operator_multiplier_half_units(
    std::uint8_t multiple_register) noexcept {
    const std::uint8_t value = static_cast<std::uint8_t>(multiple_register & 0x0fu);
    return value == 0 ? 1u : static_cast<std::uint8_t>(value * 2u);
}

inline constexpr double ym2612_operator_multiplier_ratio(
    std::uint8_t multiple_register) noexcept {
    return static_cast<double>(ym2612_operator_multiplier_half_units(multiple_register)) / 2.0;
}

inline bool ym2612_operator_is_carrier(
    std::uint8_t algorithm,
    std::size_t operator_index) {
    if (algorithm >= ym2612_algorithm_carrier_masks.size() || operator_index >= 4)
        throw std::invalid_argument("invalid YM2612 algorithm/operator index");
    return (ym2612_algorithm_carrier_masks[algorithm] & (1u << operator_index)) != 0;
}

inline ym2612_fundamental_hypothesis infer_ym2612_fundamental_hypothesis(
    const ym2612_state& chip,
    std::size_t channel_index,
    const genesis_pitch_clock_context& clocks) {
    if (channel_index >= chip.channels.size())
        throw std::invalid_argument("YM2612 fundamental hypothesis references an invalid channel");
    if (clocks.ym2612_clock_hz == 0 || clocks.source.empty())
        throw std::invalid_argument("YM2612 fundamental hypothesis requires source-relative clock provenance");

    const auto& channel = chip.channels[channel_index];
    if (channel.algorithm >= ym2612_algorithm_carrier_masks.size())
        throw std::invalid_argument("YM2612 channel contains an invalid algorithm");

    ym2612_fundamental_hypothesis result;
    result.algorithm = channel.algorithm;
    result.carrier_mask = ym2612_algorithm_carrier_masks[channel.algorithm];
    result.phase_modulation_active = chip.lfo_enabled && channel.fms != 0;

    // Channel 3 special/CSM modes have per-operator frequency semantics, so the
    // ordinary shared-FNUM hypothesis is not valid there. DAC replaces ordinary
    // FM-channel semantics on channel 6.
    if ((channel_index == 2 && chip.channel3_mode != 0) ||
        (channel_index == 5 && chip.dac_enabled) ||
        channel.fnum == 0) {
        result.detail = "ordinary shared-channel YM2612 pitch interpretation is not available";
        return result;
    }

    const auto nominal = ym2612_nominal_pitch_frequency_hz(
        channel.fnum,
        channel.block,
        clocks.ym2612_clock_hz);
    if (!nominal.has_value()) {
        result.detail = "channel FNUM/BLOCK cannot be normalized under the supplied clock";
        return result;
    }
    result.channel_pitch_interpretation_supported = true;

    bool all_operators_keyed = true;
    bool all_carriers_keyed = true;
    std::set<std::uint8_t> operator_half_units;
    std::set<std::uint8_t> carrier_half_units;

    for (std::size_t operator_index = 0; operator_index < channel.operators.size(); ++operator_index) {
        const auto& op = channel.operators[operator_index];
        const bool carrier = ym2612_operator_is_carrier(channel.algorithm, operator_index);
        const bool keyed = (channel.operator_key_mask & (1u << operator_index)) != 0;
        const std::uint8_t half_units = ym2612_operator_multiplier_half_units(op.multiple);
        const double ratio = static_cast<double>(half_units) / 2.0;

        all_operators_keyed = all_operators_keyed && keyed;
        if (carrier)
            all_carriers_keyed = all_carriers_keyed && keyed;
        result.detune_present = result.detune_present || op.detune != 0;
        result.operators.push_back({
            operator_index,
            op.multiple,
            ratio,
            *nominal * ratio,
            op.total_level,
            keyed,
            carrier,
        });
        operator_half_units.insert(half_units);
        if (carrier)
            carrier_half_units.insert(half_units);
    }

    result.all_operators_keyed = all_operators_keyed;
    result.all_carriers_keyed = all_carriers_keyed;
    for (std::uint8_t value : operator_half_units)
        result.distinct_operator_ratios.push_back(static_cast<double>(value) / 2.0);
    for (std::uint8_t value : carrier_half_units)
        result.distinct_carrier_ratios.push_back(static_cast<double>(value) / 2.0);
    result.carrier_ratios_unanimous = result.distinct_carrier_ratios.size() == 1;

    // The exact half-unit GCD is only promoted for the fully keyed static case.
    // A key-off operator can still be in envelope release, and this state model
    // does not yet track operator envelope amplitude. Partial key masks therefore
    // cannot safely define the currently audible operator lattice.
    if (result.all_operators_keyed) {
        std::uint8_t gcd_half_units = 0;
        for (std::uint8_t value : operator_half_units)
            gcd_half_units = std::gcd(gcd_half_units, value);
        if (gcd_half_units != 0) {
            result.network_periodicity_ratio = static_cast<double>(gcd_half_units) / 2.0;
            result.network_periodicity_frequency_hz =
                *nominal * *result.network_periodicity_ratio;
            result.periodicity_has_direct_carrier =
                carrier_half_units.count(gcd_half_units) != 0;
            result.missing_fundamental_case = !result.periodicity_has_direct_carrier;
        }
    }

    result.confidence = ym2612_direct_periodicity_pitch_confidence;
    if (!result.all_operators_keyed || !result.all_carriers_keyed)
        result.confidence = std::min(
            result.confidence,
            ym2612_partially_keyed_periodicity_ceiling);
    if (result.detune_present)
        result.confidence = std::min(
            result.confidence,
            ym2612_detuned_periodicity_ceiling);
    if (result.phase_modulation_active)
        result.confidence = std::min(
            result.confidence,
            ym2612_pm_periodicity_ceiling);
    if (result.missing_fundamental_case)
        result.confidence = std::min(
            result.confidence,
            ym2612_missing_fundamental_pitch_ceiling);

    if (result.network_periodicity_frequency_hz.has_value() &&
        !result.detune_present &&
        !result.phase_modulation_active &&
        result.all_operators_keyed) {
        result.performed_pitch_frequency_hz = *result.network_periodicity_frequency_hz;
        result.performed_pitch_ambiguous = false;
        if (result.periodicity_has_direct_carrier) {
            result.detail =
                "static OPN operator network has a common periodicity that is also emitted by a direct carrier; performed pitch is supported but remains distinct from heard pitch";
        } else {
            result.detail =
                "static OPN operator network has a lower common periodicity than any direct carrier; this is a missing-fundamental-style performed-pitch hypothesis and is confidence-capped";
        }
    } else if (!result.all_operators_keyed) {
        result.detail =
            "partial operator key state cannot establish the currently audible FM periodicity because release-envelope activity is not tracked";
    } else if (result.detune_present) {
        result.detail =
            "operator detune breaks the simple half-integral multiplier lattice; exact performed-pitch periodicity is unresolved";
    } else if (result.phase_modulation_active) {
        result.detail =
            "active OPN phase modulation makes the static operator-periodicity projection insufficient for one performed-pitch value";
    } else {
        result.detail = "YM2612 performed pitch remains unresolved";
    }

    return result;
}

inline std::optional<vgmtooling::model::absolute_musical_pitch_observation>
    ym2612_fundamental_as_performed_pitch(
        const ym2612_fundamental_hypothesis& hypothesis,
        vgmtooling::model::node_id source_node,
        vgmtooling::model::node_id part_id,
        vgmtooling::model::time_span active,
        std::string source) {
    using namespace vgmtooling::model;
    if (!hypothesis.performed_pitch_frequency_hz.has_value() ||
        hypothesis.performed_pitch_ambiguous || source_node == 0 || source.empty()) {
        return std::nullopt;
    }

    absolute_musical_pitch_observation result;
    result.source_node = source_node;
    result.part_id = part_id;
    result.active = std::move(active);
    result.frequency_hz = *hypothesis.performed_pitch_frequency_hz;
    result.role = musical_pitch_role::performed;
    result.status = evidence_status::hypothesis;
    result.confidence = hypothesis.confidence;
    result.source = std::move(source);
    return result;
}

} // namespace gameaudio::vgm
