#pragma once

#include "cadential_formal_closure_evidence.h"
#include "diatonic_chord_degree_hypothesis.h"
#include "harmonic_transition_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace vgmtooling::model {

// V -> VI is only harmonic morphology. A deceptive-cadence candidate requires
// that the VI arrival also coincide with independently grounded phrase
// completion. Cadence-derived phrase evidence cannot supply that closure, so
// this layer cannot prove itself circularly.
enum class ionian_deceptive_cadence_candidate_kind : std::uint8_t {
    unresolved = 0,
    deceptive_cadence_candidate,
};

struct ionian_deceptive_cadence_candidate {
    ionian_deceptive_cadence_candidate_kind kind =
        ionian_deceptive_cadence_candidate_kind::unresolved;
    time_coordinate arrival_time{};
    std::optional<std::uint8_t> source_degree{};
    std::optional<std::uint8_t> target_degree{};
    bool five_to_six_morphology = false;
    bool diatonic_morphology = false;
    bool root_motion_reliable = false;
    bool independent_phrase_completion_grounded = false;
    bool deceptive_cadence_candidate_resolved = false;
    bool cadence_class_established = false;
    bool roman_numeral_named = false;
    double morphology_confidence = 0.0;
    double formal_closure_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double ionian_deceptive_cadence_candidate_ceiling = 0.80;

inline ionian_deceptive_cadence_candidate infer_ionian_deceptive_cadence_candidate(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& source_chord,
    const tertian_triad_hypothesis& target_chord,
    const phrase_boundary_consensus& boundary) {
    if (!key.key_class_resolved || !key.mode.has_value() ||
        *key.mode != diatonic_mode::ionian) {
        throw std::invalid_argument(
            "deceptive cadence candidate requires a resolved Ionian key hypothesis");
    }

    const auto source_degree = infer_diatonic_chord_degree_hypothesis(
        key,
        source_chord);
    const auto target_degree = infer_diatonic_chord_degree_hypothesis(
        key,
        target_chord);
    const auto transition = infer_harmonic_transition(source_chord, target_chord);

    if (!compatible_phrase_boundary_time_basis(
            transition.second_time,
            boundary.representative) ||
        transition.second_time.tick != boundary.representative.tick) {
        throw std::invalid_argument(
            "deceptive cadence candidate requires phrase evidence at the V-to-VI arrival");
    }

    const auto closure = infer_cadential_formal_closure_evidence(boundary);

    ionian_deceptive_cadence_candidate result;
    result.arrival_time = transition.second_time;
    result.source_degree = source_degree.scale_degree;
    result.target_degree = target_degree.scale_degree;
    result.root_motion_reliable = transition.root_motion_reliable;
    result.diatonic_morphology =
        source_degree.scale_degree.has_value() &&
        target_degree.scale_degree.has_value() &&
        !source_degree.chromatic_root &&
        !target_degree.chromatic_root &&
        source_degree.quality_matches_diatonic_stack &&
        target_degree.quality_matches_diatonic_stack;
    result.five_to_six_morphology =
        result.diatonic_morphology &&
        result.root_motion_reliable &&
        *source_degree.scale_degree == 5 &&
        *target_degree.scale_degree == 6;
    result.independent_phrase_completion_grounded =
        closure.closure_candidate_resolved &&
        closure.noncadential_completion_grounded;
    result.morphology_confidence = std::min({
        key.confidence,
        source_degree.confidence,
        target_degree.confidence,
        transition.confidence,
        ionian_deceptive_cadence_candidate_ceiling,
    });
    result.formal_closure_confidence = closure.confidence;

    if (result.five_to_six_morphology &&
        result.independent_phrase_completion_grounded) {
        result.kind =
            ionian_deceptive_cadence_candidate_kind::deceptive_cadence_candidate;
        result.deceptive_cadence_candidate_resolved = true;
        result.confidence = std::min({
            result.morphology_confidence,
            closure.confidence,
            ionian_deceptive_cadence_candidate_ceiling,
        });
    }

    // This layer earns a bounded candidate only. Broader style, phrase-role,
    // and longer-range evidence still controls whether a final cadence class is
    // established or a Roman-numeral description is licensed for discourse.
    result.cadence_class_established = false;
    result.roman_numeral_named = false;
    return result;
}

} // namespace vgmtooling::model
