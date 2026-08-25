#pragma once

#include "ionian_deceptive_cadence_candidate.h"
#include "ionian_deferred_authentic_resolution.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace vgmtooling::model {

// Local phrase completion and longer-range continuation are different claims.
// This layer combines the two bounded V -> VI interpretations without voting
// one away. In particular, a VI arrival may be locally closing while the larger
// phrase/span continues toward a later authentic resolution.
enum class ionian_cadence_phrase_interpretation_kind : std::uint8_t {
    unresolved = 0,
    local_deceptive_close_candidate,
    continuation_candidate,
    deferred_authentic_resolution_candidate,
    local_close_with_continuation_conflict,
    local_close_with_deferred_resolution_conflict,
};

struct ionian_cadence_phrase_arbitration {
    ionian_cadence_phrase_interpretation_kind kind =
        ionian_cadence_phrase_interpretation_kind::unresolved;
    time_coordinate arrival_time{};
    bool local_deceptive_close_grounded = false;
    bool cross_part_continuation_grounded = false;
    bool deferred_authentic_resolution_grounded = false;
    bool evidence_conflict_preserved = false;
    bool phrase_role_established = false;
    bool cadence_class_established = false;
    double local_close_confidence = 0.0;
    double continuation_confidence = 0.0;
    double deferred_resolution_confidence = 0.0;
    double confidence = 0.0;
};

inline bool deferred_has_five_to_six_morphology(
    const ionian_deferred_authentic_resolution& deferred) noexcept {
    return deferred.diversion_diatonic &&
        deferred.diversion_root_motion_reliable &&
        deferred.diversion_source_degree.has_value() &&
        deferred.diversion_target_degree.has_value() &&
        *deferred.diversion_source_degree == 5 &&
        *deferred.diversion_target_degree == 6;
}

inline ionian_cadence_phrase_arbitration infer_ionian_cadence_phrase_arbitration(
    const ionian_deceptive_cadence_candidate& local_close,
    const ionian_deferred_authentic_resolution& deferred) {
    const bool local_has_morphology = local_close.five_to_six_morphology;
    const bool deferred_has_morphology = deferred_has_five_to_six_morphology(deferred);

    if (local_has_morphology && deferred_has_morphology) {
        if (!deferred_resolution_same_time_basis(
                local_close.arrival_time,
                deferred.diversion_time) ||
            local_close.arrival_time.tick != deferred.diversion_time.tick) {
            throw std::invalid_argument(
                "cadence phrase arbitration requires both V-to-VI interpretations to describe one arrival");
        }
    }

    ionian_cadence_phrase_arbitration result;
    if (local_has_morphology)
        result.arrival_time = local_close.arrival_time;
    else if (deferred_has_morphology)
        result.arrival_time = deferred.diversion_time;

    result.local_deceptive_close_grounded =
        local_close.deceptive_cadence_candidate_resolved;
    result.cross_part_continuation_grounded =
        deferred_has_morphology &&
        deferred.continuation_cross_part_grounded &&
        deferred.kind != ionian_deferred_resolution_kind::unresolved;
    result.deferred_authentic_resolution_grounded =
        result.cross_part_continuation_grounded &&
        deferred.deferred_resolution_candidate_resolved &&
        deferred.kind ==
            ionian_deferred_resolution_kind::deferred_authentic_resolution_candidate;

    result.local_close_confidence = local_close.confidence;
    result.continuation_confidence = deferred.continuation_confidence;
    result.deferred_resolution_confidence = deferred.confidence;

    if (result.local_deceptive_close_grounded &&
        result.deferred_authentic_resolution_grounded) {
        result.kind = ionian_cadence_phrase_interpretation_kind::
            local_close_with_deferred_resolution_conflict;
        result.evidence_conflict_preserved = true;
        result.confidence = std::min(
            result.local_close_confidence,
            result.deferred_resolution_confidence);
    } else if (result.local_deceptive_close_grounded &&
               result.cross_part_continuation_grounded) {
        result.kind = ionian_cadence_phrase_interpretation_kind::
            local_close_with_continuation_conflict;
        result.evidence_conflict_preserved = true;
        result.confidence = std::min(
            result.local_close_confidence,
            result.continuation_confidence);
    } else if (result.deferred_authentic_resolution_grounded) {
        result.kind = ionian_cadence_phrase_interpretation_kind::
            deferred_authentic_resolution_candidate;
        result.confidence = result.deferred_resolution_confidence;
    } else if (result.cross_part_continuation_grounded) {
        result.kind = ionian_cadence_phrase_interpretation_kind::
            continuation_candidate;
        result.confidence = result.continuation_confidence;
    } else if (result.local_deceptive_close_grounded) {
        result.kind = ionian_cadence_phrase_interpretation_kind::
            local_deceptive_close_candidate;
        result.confidence = result.local_close_confidence;
    }

    // Arbitration preserves what the current evidence says at multiple scales.
    // It does not yet know whether the arrival is phrase-final, a local close
    // inside a larger continuation, a reroute, a return, or another style-
    // specific phrase role. Those stronger claims require independent
    // longer-range evidence.
    result.phrase_role_established = false;
    result.cadence_class_established = false;
    return result;
}

} // namespace vgmtooling::model
