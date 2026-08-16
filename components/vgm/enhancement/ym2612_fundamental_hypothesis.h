#pragma once

#include "genesis_nominal_pitch.h"
#include "genesis_state.h"
#include "../../../model/harmonic_verticality.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

struct ym2612_carrier_pitch_candidate {
    std::size_t operator_index = 0;
    std::uint8_t multiple_register = 0;
    double multiplier_ratio = 1.0;
    double nominal_carrier_frequency_hz = 0.0;
    std::uint8_t total_level = 0;
    bool keyed = false;
};

struct ym2612_fundamental_hypothesis {
    std::uint8_t algorithm = 0;
    std::uint8_t carrier_mask = 0;
    std::vector<ym2612_carrier_pitch_candidate> carriers;
    std::vector<double> distinct_carrier_ratios;
    std::optional<double> strongest_performed_pitch_frequency_hz{};
    bool carrier_ratios_unanimous = false;
    bool detune_present = false;
    bool phase_modulation_active = false;
    bool all_carriers_keyed = false;
    bool channel_pitch_interpretation_supported = false;
    bool performed_pitch_ambiguous = true;
    double confidence = 0.0;
    std::string detail;
};

constexpr double ym2612_clean_carrier_pitch_confidence = 0.86;
constexpr double ym2612_detuned_carrier_pitch_ceiling = 0.72;
constexpr double ym2612_pm_carrier_pitch_ceiling = 0.64;
constexpr double ym2612_mixed_carrier_ratio_ceiling = 0.55;
constexpr double ym2612_partially_keyed_carrier_ceiling = 0.49;

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

inline constexpr double ym2612_operator_multiplier_ratio(
    std::uint8_t multiple_register) noexcept {
    const std::uint8_t value = static_cast<std::uint8_t>(multiple_register & 0x0fu);
    return value == 0 ? 0.5 : static_cast<double>(value);
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
    if ((channel_index == 2 && channel.channel3_mode != 0) ||
        (channel_index == 5 && chip.dac_enabled) ||
        channel.fnum == 0) {
        result.detail = "ordinary shared-channel YM2612 pitch interpretation is not available";
        result.confidence = 0.0;
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

    bool all_keyed = true;
    std::set<std::int64_t> quantized_ratio_keys;
    double strongest_level = 128.0;
    std::optional<double> strongest_frequency;

    for (std::size_t operator_index = 0; operator_index < channel.operators.size(); ++operator_index) {
        if (!ym2612_operator_is_carrier(channel.algorithm, operator_index))
            continue;

        const auto& op = channel.operators[operator_index];
        const double ratio = ym2612_operator_multiplier_ratio(op.multiple);
        const bool keyed = (channel.operator_key_mask & (1u << operator_index)) != 0;
        all_keyed = all_keyed && keyed;
        result.detune_present = result.detune_present || op.detune != 0;
        result.carriers.push_back({
            operator_index,
            op.multiple,
            ratio,
            *nominal * ratio,
            op.total_level,
            keyed,
        });

        // Ratio is exactly half-integral/integral in ordinary OPN MULT state.
        quantized_ratio_keys.insert(static_cast<std::int64_t>(std::llround(ratio * 2.0)));
        if (keyed && static_cast<double>(op.total_level) < strongest_level) {
            strongest_level = static_cast<double>(op.total_level);
            strongest_frequency = *nominal * ratio;
        }
    }

    result.all_carriers_keyed = all_keyed;
    for (std::int64_t key : quantized_ratio_keys)
        result.distinct_carrier_ratios.push_back(static_cast<double>(key) / 2.0);
    result.carrier_ratios_unanimous = result.distinct_carrier_ratios.size() == 1;

    result.confidence = ym2612_clean_carrier_pitch_confidence;
    if (!result.all_carriers_keyed)
        result.confidence = std::min(result.confidence, ym2612_partially_keyed_carrier_ceiling);
    if (!result.carrier_ratios_unanimous)
        result.confidence = std::min(result.confidence, ym2612_mixed_carrier_ratio_ceiling);
    if (result.detune_present)
        result.confidence = std::min(result.confidence, ym2612_detuned_carrier_pitch_ceiling);
    if (result.phase_modulation_active)
        result.confidence = std::min(result.confidence, ym2612_pm_carrier_pitch_ceiling);

    // A single shared carrier ratio gives us a bounded performed-pitch
    // hypothesis. Multiple independently audible carrier ratios describe a
    // composite FM spectrum and are not collapsed to one note merely because
    // one carrier has a lower Total Level setting.
    if (result.all_carriers_keyed && result.carrier_ratios_unanimous && strongest_frequency.has_value()) {
        result.strongest_performed_pitch_frequency_hz = *strongest_frequency;
        result.performed_pitch_ambiguous = false;
        result.detail =
            "all active algorithm carriers share one OPN multiplier ratio; performed pitch remains a hypothesis because detune/LFO/FM spectrum can alter the acoustic percept";
    } else if (!result.all_carriers_keyed) {
        result.performed_pitch_ambiguous = true;
        result.detail =
            "not every algorithm carrier is keyed, so the static algorithm topology does not establish one performed pitch";
    } else {
        result.performed_pitch_ambiguous = true;
        result.detail =
            "multiple active carrier multiplier ratios produce a composite spectrum; no single performed pitch is selected";
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
    if (!hypothesis.strongest_performed_pitch_frequency_hz.has_value() ||
        hypothesis.performed_pitch_ambiguous || source_node == 0 || source.empty()) {
        return std::nullopt;
    }

    absolute_musical_pitch_observation result;
    result.source_node = source_node;
    result.part_id = part_id;
    result.active = std::move(active);
    result.frequency_hz = *hypothesis.strongest_performed_pitch_frequency_hz;
    result.role = musical_pitch_role::performed;
    result.status = evidence_status::hypothesis;
    result.confidence = hypothesis.confidence;
    result.source = std::move(source);
    return result;
}

} // namespace gameaudio::vgm
