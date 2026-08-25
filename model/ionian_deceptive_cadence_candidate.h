#pragma once

#include "ionian_deferred_authentic_resolution.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

// Cadential deviation needs an arrival-like rhetorical cue AND evidence that
// the phrase continues through that moment. Harmonic/cadence labels are not
// allowed to provide the arrival cue here, preventing the target cadence label
// from proving itself.
struct cadential_deviation_conflict_evidence {
    time_coordinate observation_time{};
    std::vector<node_id> punctuation_supporting_parts;
    std::vector<node_id> continuation_supporting_parts;
    std::size_t punctuation_support_domains = 0;
    bool arrival_like_punctuation_grounded = false;
    bool continuation_cross_part_grounded = false;
    bool deviation_conflict_grounded = false;
    double punctuation_confidence = 0.0;
    double continuation_confidence = 0.0;
    double confidence = 0.0;
};

enum class ionian_deceptive_cadence_candidate_kind : std::uint8_t {
    unresolved = 0,
    deceptive_cadence_candidate,
};

struct ionian_deceptive_cadence_candidate {
    ionian_deceptive_cadence_candidate_kind kind =
        ionian_deceptive_cadence_candidate_kind::unresolved;
    time_coordinate diversion_time{};
    time_coordinate deferred_authentic_arrival_time{};
    bool five_to_six_diversion_grounded = false;
    bool deviation_conflict_grounded = false;
    bool deferred_authentic_resolution_grounded = false;
    bool deceptive_cadence_candidate_resolved = false;
    bool deceptive_cadence_established = false;
    bool roman_numeral_named = false;
    double deviation_conflict_confidence = 0.0;
    double deferred_resolution_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double cadential_deviation_punctuation_use_threshold = 0.70;
constexpr double cadential_deviation_conflict_ceiling = 0.78;
constexpr double ionian_deceptive_cadence_candidate_ceiling = 0.78;

inline bool noncircular_deviation_punctuation_kind(
    phrase_boundary_evidence_kind kind) noexcept {
    switch (kind) {
    case phrase_boundary_evidence_kind::temporal_gap:
    case phrase_boundary_evidence_kind::motif_completion:
    case phrase_boundary_evidence_kind::repeated_motif_alignment:
    case phrase_boundary_evidence_kind::part_density_change:
        return true;
    case phrase_boundary_evidence_kind::bass_harmonic_change:
    case phrase_boundary_evidence_kind::cadence_or_resolution:
    case phrase_boundary_evidence_kind::authored_boundary:
    case phrase_boundary_evidence_kind::driver_loop_boundary:
    case phrase_boundary_evidence_kind::external_annotation:
    case phrase_boundary_evidence_kind::cross_boundary_continuity:
        return false;
    }
    return false;
}

inline cadential_deviation_conflict_evidence infer_cadential_deviation_conflict_evidence(
    const std::vector<part_phrase_boundary_hypothesis>& part_hypotheses,
    const time_coordinate& observation_time) {
    const auto continuation = infer_cross_part_continuation_evidence(
        part_hypotheses,
        observation_time);

    std::map<node_id, double> punctuation_by_part;
    std::map<phrase_boundary_evidence_origin, double> punctuation_by_domain;

    for (const auto& item : part_hypotheses) {
        if (item.part_id == 0)
            throw std::invalid_argument(
                "cadential deviation conflict requires nonzero persistent-part ids");
        if (!compatible_phrase_boundary_time_basis(
                observation_time,
                item.boundary.boundary) ||
            observation_time.tick != item.boundary.boundary.tick) {
            throw std::invalid_argument(
                "cadential deviation conflict must describe one exact observation time");
        }

        double best_part = 0.0;
        for (const auto& evidence : item.boundary.evidence) {
            validate_phrase_boundary_evidence(evidence);
            if (evidence.polarity != phrase_boundary_evidence_polarity::supports ||
                !noncircular_deviation_punctuation_kind(evidence.kind) ||
                evidence.confidence < cadential_deviation_punctuation_use_threshold) {
                continue;
            }
            best_part = std::max(best_part, evidence.confidence);
            punctuation_by_domain[evidence.origin] = std::max(
                punctuation_by_domain[evidence.origin],
                evidence.confidence);
        }
        if (best_part > 0.0) {
            auto found = punctuation_by_part.find(item.part_id);
            if (found == punctuation_by_part.end())
                punctuation_by_part.emplace(item.part_id, best_part);
            else
                found->second = std::max(found->second, best_part);
        }
    }

    cadential_deviation_conflict_evidence result;
    result.observation_time = observation_time;
    result.continuation_supporting_parts = continuation.supporting_parts;
    result.continuation_cross_part_grounded = continuation.cross_part_grounded;
    result.continuation_confidence = continuation.confidence;
    result.punctuation_support_domains = punctuation_by_domain.size();
    for (const auto& item : punctuation_by_part)
        result.punctuation_supporting_parts.push_back(item.first);

    if (punctuation_by_part.size() >= 2 && punctuation_by_domain.size() >= 2) {
        std::vector<double> part_strengths;
        for (const auto& item : punctuation_by_part)
            part_strengths.push_back(item.second);
        std::sort(part_strengths.begin(), part_strengths.end(), std::greater<double>{});

        std::vector<double> domain_strengths;
        for (const auto& item : punctuation_by_domain)
            domain_strengths.push_back(item.second);
        std::sort(domain_strengths.begin(), domain_strengths.end(), std::greater<double>{});

        result.arrival_like_punctuation_grounded = true;
        result.punctuation_confidence = std::min({
            part_strengths[1],
            domain_strengths[1],
            cadential_deviation_conflict_ceiling,
        });
    }

    if (result.arrival_like_punctuation_grounded &&
        result.continuation_cross_part_grounded) {
        result.deviation_conflict_grounded = true;
        result.confidence = std::min({
            result.punctuation_confidence,
            result.continuation_confidence,
            cadential_deviation_conflict_ceiling,
        });
    }
    return result;
}

inline ionian_deceptive_cadence_candidate infer_ionian_deceptive_cadence_candidate(
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
    const auto deferred = infer_ionian_deferred_authentic_resolution(
        key,
        diversion_source_chord,
        diversion_target_chord,
        diversion_part_hypotheses,
        later_dominant_chord,
        later_tonic_chord,
        later_arrival,
        later_boundary,
        later_voices,
        later_bass,
        later_melodic);

    const auto conflict = infer_cadential_deviation_conflict_evidence(
        diversion_part_hypotheses,
        deferred.diversion_time);

    ionian_deceptive_cadence_candidate result;
    result.diversion_time = deferred.diversion_time;
    result.deferred_authentic_arrival_time = deferred.later_authentic_arrival_time;
    result.five_to_six_diversion_grounded =
        deferred.diversion_source_degree.has_value() &&
        deferred.diversion_target_degree.has_value() &&
        *deferred.diversion_source_degree == 5 &&
        *deferred.diversion_target_degree == 6 &&
        deferred.diversion_diatonic &&
        deferred.diversion_root_motion_reliable;
    result.deviation_conflict_grounded = conflict.deviation_conflict_grounded;
    result.deferred_authentic_resolution_grounded =
        deferred.deferred_resolution_candidate_resolved;
    result.deviation_conflict_confidence = conflict.confidence;
    result.deferred_resolution_confidence = deferred.confidence;

    if (result.five_to_six_diversion_grounded &&
        result.deviation_conflict_grounded &&
        result.deferred_authentic_resolution_grounded) {
        result.kind = ionian_deceptive_cadence_candidate_kind::deceptive_cadence_candidate;
        result.deceptive_cadence_candidate_resolved = true;
        result.confidence = std::min({
            conflict.confidence,
            deferred.confidence,
            ionian_deceptive_cadence_candidate_ceiling,
        });
    }

    // Candidate naming is now justified by a specific common-practice evidence
    // pattern, but it still remains a hypothesis rather than an established
    // cadence class. Style transfer beyond this narrow Ionian scope comes later.
    result.deceptive_cadence_established = false;
    result.roman_numeral_named = false;
    return result;
}

} // namespace vgmtooling::model
