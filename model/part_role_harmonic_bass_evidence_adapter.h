#pragma once

#include "bass_harmony_interaction.h"
#include "part_role_window_inference.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

// Bass ownership is a time-local harmonic relation, not a synonym for
// "currently lowest pitch" and not a synonym for chord root. The evidence
// admitted here has already passed through successive harmonic verticalities,
// persistent-part voice correspondence, and bass/harmony interaction analysis.
// Requiring repeated grounded transitions keeps a one-off low passing tone or
// stale held note from becoming a bass-foundation role by register alone.
struct harmonic_bass_role_evidence_policy {
    std::size_t full_support_transitions = 3;
};

struct harmonic_bass_ownership_summary {
    std::size_t eligible_transitions = 0;
    std::size_t grounded_transitions = 0;
    std::size_t owned_transitions = 0;
    double ownership_fraction = 0.0;
    double confidence = 0.0;
};

inline bool harmonic_bass_time_inside_window(
    const time_coordinate& coordinate,
    const time_span& window) noexcept {
    return window.end.has_value() &&
        part_role_same_time_basis(coordinate, window.start) &&
        coordinate.tick >= window.start.tick &&
        coordinate.tick < window.end->tick;
}

inline void validate_bass_harmony_interaction_for_role(
    const bass_harmony_interaction_hypothesis& interaction) {
    if (!std::isfinite(interaction.confidence) ||
        interaction.confidence < 0.0 || interaction.confidence > 1.0) {
        throw std::invalid_argument(
            "bass-role interaction confidence must be finite in [0, 1]");
    }
    if (!part_role_same_time_basis(interaction.first_time, interaction.second_time) ||
        interaction.second_time.tick <= interaction.first_time.tick) {
        throw std::invalid_argument(
            "bass-role interaction requires an ordered transition in one time basis");
    }
    if (interaction.bass_identity_grounded && interaction.bass_part_id == 0) {
        throw std::invalid_argument(
            "grounded bass-role interaction requires a persistent-part id");
    }
}

inline harmonic_bass_ownership_summary infer_harmonic_bass_ownership(
    const part_role_window_descriptor& descriptor,
    const std::vector<bass_harmony_interaction_hypothesis>& interactions,
    harmonic_bass_role_evidence_policy policy = {}) {
    validate_part_role_window_descriptor(descriptor);
    if (policy.full_support_transitions == 0)
        throw std::invalid_argument("bass-role evidence needs a nonzero support horizon");

    harmonic_bass_ownership_summary summary;
    double grounded_weight = 0.0;
    double owned_weight = 0.0;
    double grounded_confidence_sum = 0.0;

    for (const auto& interaction : interactions) {
        validate_bass_harmony_interaction_for_role(interaction);

        // A transition contributes to this role window only when the complete
        // harmonic change lies inside it. Partial overlap is unknown rather than
        // silently clipped into stronger evidence.
        if (!harmonic_bass_time_inside_window(interaction.first_time, descriptor.active) ||
            !harmonic_bass_time_inside_window(interaction.second_time, descriptor.active)) {
            continue;
        }

        ++summary.eligible_transitions;
        if (!interaction.bass_identity_grounded ||
            interaction.bass_part_id == 0 ||
            interaction.kind == bass_harmony_interaction_kind::unresolved ||
            !(interaction.confidence > 0.0)) {
            continue;
        }

        ++summary.grounded_transitions;
        grounded_weight += interaction.confidence;
        grounded_confidence_sum += interaction.confidence;
        if (interaction.bass_part_id == descriptor.part_id) {
            ++summary.owned_transitions;
            owned_weight += interaction.confidence;
        }
    }

    if (summary.grounded_transitions == 0 || !(grounded_weight > 0.0))
        return summary;

    summary.ownership_fraction = std::clamp(
        owned_weight / grounded_weight,
        0.0,
        1.0);

    const double mean_grounded_confidence =
        grounded_confidence_sum / static_cast<double>(summary.grounded_transitions);
    const double support_maturity = std::min(
        1.0,
        static_cast<double>(summary.grounded_transitions) /
            static_cast<double>(policy.full_support_transitions));
    const double grounded_coverage = summary.eligible_transitions == 0
        ? 0.0
        : static_cast<double>(summary.grounded_transitions) /
            static_cast<double>(summary.eligible_transitions);

    summary.confidence = std::clamp(
        mean_grounded_confidence * support_maturity * grounded_coverage,
        0.0,
        1.0);
    return summary;
}

inline bool attach_harmonic_bass_ownership(
    part_role_window_descriptor& descriptor,
    const std::vector<bass_harmony_interaction_hypothesis>& interactions,
    harmonic_bass_role_evidence_policy policy = {}) {
    const auto summary = infer_harmonic_bass_ownership(
        descriptor,
        interactions,
        policy);
    if (summary.grounded_transitions == 0)
        return false;

    descriptor.harmonic_bass_ownership = bounded_role_signal{
        summary.ownership_fraction,
        summary.confidence,
    };
    return true;
}

} // namespace vgmtooling::model
