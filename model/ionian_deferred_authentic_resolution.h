#pragma once

#include "ionian_cadence_formal_binding.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

// A V -> VI progression is not, by itself, a deceptive cadence. This layer
// looks for a more specific observed pattern: a diatonic 5 -> 6 diversion that
// does NOT close the phrase, followed later by an independently grounded
// authentic-cadence candidate. Even then we name only deferred authentic
// resolution, not an established deceptive cadence.
enum class ionian_deferred_resolution_kind : std::uint8_t {
    unresolved = 0,
    five_to_six_continuation_candidate,
    deferred_authentic_resolution_candidate,
};

struct cross_part_continuation_evidence {
    time_coordinate observation_time{};
    std::vector<node_id> supporting_parts;
    bool cross_part_grounded = false;
    double confidence = 0.0;
};

struct ionian_deferred_authentic_resolution {
    ionian_deferred_resolution_kind kind = ionian_deferred_resolution_kind::unresolved;
    time_coordinate diversion_time{};
    time_coordinate later_authentic_arrival_time{};
    std::optional<std::uint8_t> diversion_source_degree{};
    std::optional<std::uint8_t> diversion_target_degree{};
    bool diversion_diatonic = false;
    bool diversion_root_motion_reliable = false;
    bool continuation_cross_part_grounded = false;
    bool later_authentic_candidate_grounded = false;
    ionian_cadence_candidate_kind later_authentic_kind =
        ionian_cadence_candidate_kind::unresolved;
    bool deferred_resolution_candidate_resolved = false;
    bool deceptive_cadence_named = false;
    bool cadence_class_established = false;
    double diversion_confidence = 0.0;
    double continuation_confidence = 0.0;
    double later_authentic_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double cadential_continuation_use_threshold = 0.70;
constexpr double cross_part_continuation_ceiling = 0.82;
constexpr double deferred_authentic_resolution_ceiling = 0.82;

inline bool deferred_resolution_same_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline cross_part_continuation_evidence infer_cross_part_continuation_evidence(
    const std::vector<part_phrase_boundary_hypothesis>& part_hypotheses,
    const time_coordinate& observation_time) {
    if (part_hypotheses.empty())
        throw std::invalid_argument(
            "cross-part continuation requires persistent-part boundary hypotheses");

    std::map<node_id, double> best_part_confidence;
    for (const auto& item : part_hypotheses) {
        if (item.part_id == 0)
            throw std::invalid_argument(
                "cross-part continuation requires nonzero persistent-part ids");
        if (!compatible_phrase_boundary_time_basis(
                observation_time,
                item.boundary.boundary) ||
            observation_time.tick != item.boundary.boundary.tick) {
            throw std::invalid_argument(
                "cross-part continuation evidence must describe one exact observation time");
        }

        double best = 0.0;
        for (const auto& evidence : item.boundary.evidence) {
            validate_phrase_boundary_evidence(evidence);
            if (evidence.kind != phrase_boundary_evidence_kind::cross_boundary_continuity ||
                evidence.polarity != phrase_boundary_evidence_polarity::counters ||
                evidence.confidence < cadential_continuation_use_threshold) {
                continue;
            }
            best = std::max(best, evidence.confidence);
        }
        if (best > 0.0) {
            auto found = best_part_confidence.find(item.part_id);
            if (found == best_part_confidence.end())
                best_part_confidence.emplace(item.part_id, best);
            else
                found->second = std::max(found->second, best);
        }
    }

    cross_part_continuation_evidence result;
    result.observation_time = observation_time;
    for (const auto& item : best_part_confidence)
        result.supporting_parts.push_back(item.first);

    if (best_part_confidence.size() < 2)
        return result;

    std::vector<double> strengths;
    strengths.reserve(best_part_confidence.size());
    for (const auto& item : best_part_confidence)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});

    result.cross_part_grounded = true;
    result.confidence = std::min(
        strengths[1],
        cross_part_continuation_ceiling);
    return result;
}

inline bool authentic_morphology_kind(
    ionian_cadence_candidate_kind kind) noexcept {
    return kind == ionian_cadence_candidate_kind::authentic_cadence_candidate ||
        kind == ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate ||
        kind == ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate;
}

inline ionian_deferred_authentic_resolution infer_ionian_deferred_authentic_resolution(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& diversion_source_chord,
    const tertian_triad_hypothesis& diversion_target_chord,
    const std::vector<part_phrase_boundary_hypothesis>& diversion_part_hypotheses,
    const tertian_triad_hypothesis& later_dominant_chord,
    const tertian_triad_hypothesis& later_tonic_chord,
    const cadential_arrival_hypothesis& later_arrival,
    const phrase_boundary_consensus& later_boundary,
    const std::optional<voice_leading_hypothesis>& later_voices = std::nullopt,
    const std::optional<bass_harmony_interaction_hypothesis>& later_bass = std::nullopt,
    const std::optional<cadential_melodic_arrival_evidence>& later_melodic = std::nullopt) {
    if (!key.key_class_resolved || !key.mode.has_value() || *key.mode != diatonic_mode::ionian)
        throw std::invalid_argument(
            "deferred authentic resolution requires a resolved Ionian key hypothesis");

    const auto source_degree = infer_diatonic_chord_degree_hypothesis(
        key,
        diversion_source_chord);
    const auto target_degree = infer_diatonic_chord_degree_hypothesis(
        key,
        diversion_target_chord);
    const auto diversion_transition = infer_harmonic_transition(
        diversion_source_chord,
        diversion_target_chord);

    ionian_deferred_authentic_resolution result;
    result.diversion_time = diversion_transition.second_time;
    result.diversion_source_degree = source_degree.scale_degree;
    result.diversion_target_degree = target_degree.scale_degree;
    result.diversion_root_motion_reliable = diversion_transition.root_motion_reliable;
    result.diversion_diatonic =
        source_degree.scale_degree.has_value() &&
        target_degree.scale_degree.has_value() &&
        !source_degree.chromatic_root &&
        !target_degree.chromatic_root &&
        source_degree.quality_matches_diatonic_stack &&
        target_degree.quality_matches_diatonic_stack;
    result.diversion_confidence = std::min({
        key.confidence,
        source_degree.confidence,
        target_degree.confidence,
        diversion_transition.confidence,
        deferred_authentic_resolution_ceiling,
    });

    const auto continuation = infer_cross_part_continuation_evidence(
        diversion_part_hypotheses,
        diversion_transition.second_time);
    result.continuation_cross_part_grounded = continuation.cross_part_grounded;
    result.continuation_confidence = continuation.confidence;

    const bool is_five_to_six =
        result.diversion_diatonic &&
        result.diversion_root_motion_reliable &&
        result.diversion_source_degree.has_value() &&
        result.diversion_target_degree.has_value() &&
        *result.diversion_source_degree == 5 &&
        *result.diversion_target_degree == 6;

    if (!is_five_to_six || !continuation.cross_part_grounded)
        return result;

    result.kind = ionian_deferred_resolution_kind::five_to_six_continuation_candidate;
    result.confidence = std::min({
        result.diversion_confidence,
        continuation.confidence,
        deferred_authentic_resolution_ceiling,
    });

    const auto later_dominant_time =
        later_dominant_chord.projection.source_verticality.observation_time;
    if (!deferred_resolution_same_time_basis(
            diversion_transition.second_time,
            later_dominant_time) ||
        later_dominant_time.tick <= diversion_transition.second_time.tick) {
        throw std::invalid_argument(
            "deferred authentic resolution requires the later dominant to follow the diversion");
    }

    const auto later_binding = infer_ionian_cadence_formal_binding(
        key,
        later_dominant_chord,
        later_tonic_chord,
        later_arrival,
        later_boundary,
        later_voices,
        later_bass,
        later_melodic);

    if (!deferred_resolution_same_time_basis(
            diversion_transition.second_time,
            later_binding.arrival_time) ||
        later_binding.arrival_time.tick <= diversion_transition.second_time.tick) {
        throw std::invalid_argument(
            "deferred authentic resolution requires a later cadence event in the same time basis");
    }

    result.later_authentic_arrival_time = later_binding.arrival_time;
    result.later_authentic_kind = later_binding.morphology_kind;
    result.later_authentic_candidate_grounded =
        later_binding.integrated_cadence_candidate_resolved &&
        authentic_morphology_kind(later_binding.morphology_kind);
    result.later_authentic_confidence = later_binding.confidence;

    if (result.later_authentic_candidate_grounded) {
        result.kind = ionian_deferred_resolution_kind::deferred_authentic_resolution_candidate;
        result.deferred_resolution_candidate_resolved = true;
        result.confidence = std::min({
            result.confidence,
            later_binding.confidence,
            deferred_authentic_resolution_ceiling,
        });
    }

    // This is evidence of deferred authentic resolution, not permission to
    // rename every V -> VI continuation as a deceptive cadence. Stronger style-
    // and grouping-specific evidence is still required for that final label.
    result.deceptive_cadence_named = false;
    result.cadence_class_established = false;
    return result;
}

} // namespace vgmtooling::model
