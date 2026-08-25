#pragma once

#include "cadential_formal_closure_evidence.h"
#include "ionian_cadence_class_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace vgmtooling::model {

// Keep cadence morphology and formal closure as separate, traceable facts.
// This binding only says that both independently grounded stories describe the
// same arrival. It does not promote a candidate into an established cadence
// class, Roman numeral, or formal-function label.
struct ionian_cadence_formal_binding {
    time_coordinate arrival_time{};
    ionian_cadence_candidate_kind morphology_kind =
        ionian_cadence_candidate_kind::unresolved;
    bool morphology_candidate_resolved = false;
    bool formal_closure_candidate_resolved = false;
    bool integrated_cadence_candidate_resolved = false;
    bool cadence_class_established = false;
    bool roman_numeral_named = false;
    double morphology_confidence = 0.0;
    double formal_closure_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double ionian_integrated_cadence_candidate_ceiling = 0.82;

inline ionian_cadence_formal_binding infer_ionian_cadence_formal_binding(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& first_chord,
    const tertian_triad_hypothesis& second_chord,
    const cadential_arrival_hypothesis& arrival,
    const phrase_boundary_consensus& boundary,
    const std::optional<voice_leading_hypothesis>& voices = std::nullopt,
    const std::optional<bass_harmony_interaction_hypothesis>& bass = std::nullopt,
    const std::optional<cadential_melodic_arrival_evidence>& melodic = std::nullopt) {
    if (!same_function_transition_time(boundary.representative, arrival.arrival_time))
        throw std::invalid_argument(
            "cadence formal binding requires the same phrase-boundary arrival time");

    const bool boundary_global_grounding =
        boundary.cross_part_grounded || boundary.authored_grounded;
    if (boundary_global_grounding != arrival.cross_part_phrase_grounded)
        throw std::invalid_argument(
            "cadence formal binding phrase grounding disagrees with the arrival witness");
    if (!std::isfinite(arrival.phrase_boundary_confidence) ||
        std::fabs(arrival.phrase_boundary_confidence - boundary.confidence) > 1e-12) {
        throw std::invalid_argument(
            "cadence formal binding phrase confidence disagrees with the arrival witness");
    }

    const auto morphology = infer_ionian_cadence_class_hypothesis(
        key,
        first_chord,
        second_chord,
        arrival,
        voices,
        bass,
        melodic);
    const auto closure = infer_cadential_formal_closure_evidence(boundary);

    ionian_cadence_formal_binding result;
    result.arrival_time = arrival.arrival_time;
    result.morphology_kind = morphology.kind;
    result.morphology_candidate_resolved = morphology.cadence_candidate_resolved;
    result.formal_closure_candidate_resolved = closure.closure_candidate_resolved;
    result.morphology_confidence = morphology.confidence;
    result.formal_closure_confidence = closure.confidence;

    if (result.morphology_candidate_resolved &&
        result.formal_closure_candidate_resolved) {
        result.integrated_cadence_candidate_resolved = true;
        result.confidence = std::min({
            morphology.confidence,
            closure.confidence,
            ionian_integrated_cadence_candidate_ceiling,
        });
    }

    result.cadence_class_established = false;
    result.roman_numeral_named = false;
    return result;
}

} // namespace vgmtooling::model
