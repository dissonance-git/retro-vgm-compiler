#pragma once

#include "harmonic_verticality.h"
#include "tuning_projection.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class symbolic_projection_loss : std::uint32_t {
    none = 0,
    pitch_quantized = 1u << 0u,
    timbre_unrepresented = 1u << 1u,
    dynamics_unrepresented = 1u << 2u,
    articulation_unrepresented = 1u << 3u,
    physical_channel_unrepresented = 1u << 4u,
    native_control_state_unrepresented = 1u << 5u,
    loop_branch_structure_unrepresented = 1u << 6u,
    meter_tempo_not_established = 1u << 7u,
    persistent_part_unknown = 1u << 8u,
    percussion_identity_uncertain = 1u << 9u,
};

using symbolic_projection_losses = std::uint32_t;

constexpr symbolic_projection_losses to_losses(symbolic_projection_loss loss) noexcept {
    return static_cast<symbolic_projection_losses>(loss);
}

constexpr symbolic_projection_losses operator|(
    symbolic_projection_loss first,
    symbolic_projection_loss second) noexcept {
    return to_losses(first) | to_losses(second);
}

constexpr symbolic_projection_losses operator|(
    symbolic_projection_losses first,
    symbolic_projection_loss second) noexcept {
    return first | to_losses(second);
}

constexpr bool has_loss(
    symbolic_projection_losses losses,
    symbolic_projection_loss loss) noexcept {
    return (losses & to_losses(loss)) != 0;
}

struct symbolic_note_projection_policy {
    equal_temperament_model tuning{};
    double maximum_deviation_cents = 35.0;
    bool require_bounded_duration = true;
    bool expose_midi_note_view = true;
};

struct symbolic_note_projection {
    node_id source_node = 0;
    node_id part_id = 0;
    time_span active{};
    musical_pitch_role pitch_role = musical_pitch_role::programmed;
    double source_frequency_hz = 0.0;

    // Generic equal-temperament coordinate. This remains meaningful for
    // non-12TET projections; MIDI note numbers do not.
    std::int64_t tuning_step = 0;
    double tuning_deviation_cents = 0.0;

    // Present only when the supplied tuning is 12-TET and the projected step
    // lies in the standard MIDI note-number range. This is an export view, not
    // the identity of the underlying musical pitch.
    std::optional<std::uint8_t> midi_note{};

    double confidence = 0.0;
    symbolic_projection_losses losses = to_losses(symbolic_projection_loss::none);
    std::vector<provenance_ref> provenance;
};

inline symbolic_note_projection make_symbolic_note_projection(
    const absolute_musical_pitch_observation& observation,
    symbolic_note_projection_policy policy) {
    validate_absolute_musical_pitch_observation(observation);
    validate_equal_temperament_model(policy.tuning);
    if (!std::isfinite(policy.maximum_deviation_cents) ||
        policy.maximum_deviation_cents <= 0.0) {
        throw std::invalid_argument(
            "symbolic-note pitch tolerance must be finite and positive");
    }
    if (policy.require_bounded_duration && !observation.active.end.has_value()) {
        throw std::invalid_argument(
            "bounded symbolic-note projection requires an established note end");
    }

    const double divisions = static_cast<double>(policy.tuning.divisions_per_octave);
    const double exact_step =
        static_cast<double>(policy.tuning.reference_step) +
        divisions * std::log2(
            observation.frequency_hz / policy.tuning.reference_frequency_hz);
    const std::int64_t nearest_step =
        static_cast<std::int64_t>(std::llround(exact_step));
    const double deviation_cents =
        (exact_step - static_cast<double>(nearest_step)) * 1200.0 / divisions;
    const double absolute_deviation = std::fabs(deviation_cents);
    if (absolute_deviation > policy.maximum_deviation_cents) {
        throw std::invalid_argument(
            "musical pitch does not fit the supplied symbolic tuning within tolerance");
    }

    const double fit = std::max(
        0.0,
        1.0 - absolute_deviation / policy.maximum_deviation_cents);

    symbolic_note_projection result;
    result.source_node = observation.source_node;
    result.part_id = observation.part_id;
    result.active = observation.active;
    result.pitch_role = observation.role;
    result.source_frequency_hz = observation.frequency_hz;
    result.tuning_step = nearest_step;
    result.tuning_deviation_cents = deviation_cents;
    result.confidence = std::min({
        observation.confidence,
        policy.tuning.confidence,
        fit,
    });

    // A note-number view cannot carry the native synthesis/program/sample
    // state by itself. Those losses are declared even for an exact 12-TET
    // pitch match.
    result.losses =
        symbolic_projection_loss::timbre_unrepresented |
        symbolic_projection_loss::dynamics_unrepresented |
        symbolic_projection_loss::articulation_unrepresented |
        symbolic_projection_loss::physical_channel_unrepresented |
        symbolic_projection_loss::native_control_state_unrepresented |
        symbolic_projection_loss::loop_branch_structure_unrepresented |
        symbolic_projection_loss::meter_tempo_not_established;

    if (absolute_deviation > 1e-9)
        result.losses = result.losses | symbolic_projection_loss::pitch_quantized;
    if (observation.part_id == 0)
        result.losses = result.losses | symbolic_projection_loss::persistent_part_unknown;

    if (policy.expose_midi_note_view &&
        policy.tuning.divisions_per_octave == 12 &&
        nearest_step >= 0 && nearest_step <= 127) {
        result.midi_note = static_cast<std::uint8_t>(nearest_step);
    }

    result.provenance.push_back({
        observation.status,
        observation.confidence,
        observation.source,
        std::nullopt,
        "absolute musical pitch evidence used as the source of this symbolic note projection",
    });
    result.provenance.push_back({
        evidence_status::hypothesis,
        policy.tuning.confidence,
        policy.tuning.source,
        std::nullopt,
        "explicit equal-temperament model used for symbolic pitch quantization; exported note number is a projection, not source truth",
    });
    return result;
}

} // namespace vgmtooling::model
