#pragma once

#include "bass_harmony_interaction.h"
#include "cadential_arrival_hypothesis.h"
#include "tonal_center_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

inline bool compatible_tonal_center_tuning(
    const equal_temperament_model& first,
    const equal_temperament_model& second) noexcept {
    return first.divisions_per_octave == second.divisions_per_octave &&
           first.reference_step == second.reference_step &&
           std::fabs(first.reference_frequency_hz - second.reference_frequency_hz) <= 1e-9;
}

inline double equal_temperament_step_octave_class(
    const equal_temperament_model& tuning,
    std::int64_t step) {
    validate_equal_temperament_model(tuning);
    const double divisions = static_cast<double>(tuning.divisions_per_octave);
    const double log2_frequency =
        std::log2(tuning.reference_frequency_hz) +
        (static_cast<double>(step - tuning.reference_step) / divisions);
    return normalize_octave_class(log2_frequency);
}

inline double triad_root_octave_class(const tertian_triad_hypothesis& chord) {
    if (chord.projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("tonal-center triad adapter requires explicit 12-TET projection");
    if (chord.root_ambiguous)
        throw std::invalid_argument("ambiguous chord root cannot support a tonal-center root claim");
    return equal_temperament_step_octave_class(
        chord.projection.tuning,
        chord.root_pitch_class);
}

inline tonal_center_evidence make_harmonic_stability_center_evidence(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    const harmonic_transition_hypothesis& transition,
    std::string dependency_group) {
    if (dependency_group.empty())
        throw std::invalid_argument("harmonic tonal-center evidence requires an explicit dependency group");
    if (!compatible_tonal_center_tuning(first.projection.tuning, second.projection.tuning))
        throw std::invalid_argument("harmonic tonal-center evidence requires one tuning contract");
    if (!transition.root_motion_reliable)
        throw std::invalid_argument("unreliable harmonic root motion cannot establish center stability");
    if (first.root_ambiguous || second.root_ambiguous)
        throw std::invalid_argument("ambiguous chord roots cannot establish center stability");
    if (first.root_pitch_class != second.root_pitch_class ||
        transition.directed_root_motion_semitones != 0) {
        throw std::invalid_argument("harmonic stability evidence requires retained chord root");
    }
    if (!compatible_phrase_boundary_time_basis(
            first.projection.source_verticality.observation_time,
            transition.first_time) ||
        !compatible_phrase_boundary_time_basis(
            second.projection.source_verticality.observation_time,
            transition.second_time) ||
        first.projection.source_verticality.observation_time.tick != transition.first_time.tick ||
        second.projection.source_verticality.observation_time.tick != transition.second_time.tick) {
        throw std::invalid_argument("harmonic stability evidence must describe the supplied chord transition");
    }

    tonal_center_evidence evidence;
    evidence.kind = tonal_center_evidence_kind::harmonic_stability;
    evidence.origin = tonal_center_evidence_origin::harmony;
    evidence.center_octave_class = triad_root_octave_class(second);
    evidence.confidence = std::min({first.confidence, second.confidence, transition.confidence});
    evidence.dependency_group = std::move(dependency_group);
    evidence.status = evidence_status::hypothesis;
    evidence.source = "retained unambiguous chord root under explicit tuning projection";
    return evidence;
}

inline tonal_center_evidence make_bass_support_center_evidence(
    const tertian_triad_hypothesis& chord,
    const bass_harmony_interaction_hypothesis& interaction,
    std::string dependency_group) {
    if (dependency_group.empty())
        throw std::invalid_argument("bass tonal-center evidence requires an explicit dependency group");
    if (!interaction.bass_identity_grounded || interaction.bass_part_id == 0)
        throw std::invalid_argument("bass tonal-center evidence requires grounded persistent bass identity");
    if (chord.inversion != triad_inversion::root_position)
        throw std::invalid_argument("bass tonal-center evidence requires the grounded bass to carry the chord root");
    if (chord.root_ambiguous)
        throw std::invalid_argument("ambiguous chord root cannot establish bass center support");
    if (chord.projection.nearest_steps.empty() || chord.projection.source_verticality.part_ids.empty())
        throw std::invalid_argument("bass tonal-center evidence requires projected bass pitch and part identity");
    if (chord.projection.source_verticality.part_ids.front() != interaction.bass_part_id)
        throw std::invalid_argument("bass tonal-center evidence references a different persistent bass part");

    const double root_center = triad_root_octave_class(chord);
    const double bass_center = equal_temperament_step_octave_class(
        chord.projection.tuning,
        chord.projection.nearest_steps.front());
    if (circular_octave_class_distance(root_center, bass_center) > 1e-9)
        throw std::invalid_argument("root-position bass projection does not match the inferred chord root");

    tonal_center_evidence evidence;
    evidence.kind = tonal_center_evidence_kind::bass_support;
    evidence.origin = tonal_center_evidence_origin::bass_structure;
    evidence.center_octave_class = bass_center;
    evidence.confidence = std::min(chord.confidence, interaction.confidence);
    evidence.dependency_group = std::move(dependency_group);
    evidence.status = evidence_status::hypothesis;
    evidence.source = "grounded persistent bass carrying an unambiguous root-position chord root";
    return evidence;
}

inline tonal_center_evidence make_structural_arrival_center_evidence(
    const cadential_arrival_hypothesis& arrival,
    const tertian_triad_hypothesis& terminal_chord,
    std::string dependency_group) {
    if (dependency_group.empty())
        throw std::invalid_argument("arrival tonal-center evidence requires an explicit dependency group");
    if (terminal_chord.root_ambiguous)
        throw std::invalid_argument("ambiguous terminal chord root cannot support a center arrival");
    const auto& chord_time = terminal_chord.projection.source_verticality.observation_time;
    if (!compatible_phrase_boundary_time_basis(arrival.arrival_time, chord_time) ||
        arrival.arrival_time.tick != chord_time.tick) {
        throw std::invalid_argument("tonal-center arrival evidence must align with the terminal chord");
    }

    tonal_center_evidence evidence;
    evidence.kind = tonal_center_evidence_kind::structural_arrival;
    evidence.origin = tonal_center_evidence_origin::phrase_structure;
    evidence.center_octave_class = triad_root_octave_class(terminal_chord);
    evidence.confidence = std::min(arrival.confidence, terminal_chord.confidence);
    evidence.dependency_group = std::move(dependency_group);
    evidence.status = evidence_status::hypothesis;
    evidence.source = "phrase-aligned structural arrival at an unambiguous terminal chord root";
    return evidence;
}

} // namespace vgmtooling::model
