#include "model/ionian_cadence_phrase_arbitration.h"

#include <cmath>
#include <stdexcept>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

ionian_deceptive_cadence_candidate local_close(double confidence = 0.78) {
    ionian_deceptive_cadence_candidate result;
    result.kind = ionian_deceptive_cadence_candidate_kind::deceptive_cadence_candidate;
    result.arrival_time = at(200);
    result.source_degree = 5;
    result.target_degree = 6;
    result.five_to_six_morphology = true;
    result.diatonic_morphology = true;
    result.root_motion_reliable = true;
    result.independent_phrase_completion_grounded = true;
    result.deceptive_cadence_candidate_resolved = true;
    result.confidence = confidence;
    return result;
}

ionian_deferred_authentic_resolution continuation(
    bool reaches_later_authentic,
    double continuation_confidence = 0.79,
    double confidence = 0.80) {
    ionian_deferred_authentic_resolution result;
    result.kind = reaches_later_authentic
        ? ionian_deferred_resolution_kind::deferred_authentic_resolution_candidate
        : ionian_deferred_resolution_kind::five_to_six_continuation_candidate;
    result.diversion_time = at(200);
    result.later_authentic_arrival_time = at(400);
    result.diversion_source_degree = 5;
    result.diversion_target_degree = 6;
    result.diversion_diatonic = true;
    result.diversion_root_motion_reliable = true;
    result.continuation_cross_part_grounded = true;
    result.later_authentic_candidate_grounded = reaches_later_authentic;
    result.deferred_resolution_candidate_resolved = reaches_later_authentic;
    result.continuation_confidence = continuation_confidence;
    result.confidence = confidence;
    return result;
}

ionian_deceptive_cadence_candidate unresolved_local() {
    ionian_deceptive_cadence_candidate result;
    result.arrival_time = at(200);
    result.source_degree = 5;
    result.target_degree = 6;
    result.five_to_six_morphology = true;
    result.diatonic_morphology = true;
    result.root_motion_reliable = true;
    return result;
}
} // namespace

int main() {
    // Independent local closure and later authentic resolution are not forced
    // into one label. The conflict survives so a later phrase-role layer can
    // decide whether the VI arrival closes locally, continues globally, or both.
    const auto local = local_close();
    const auto deferred = continuation(true);
    const auto conflict = infer_ionian_cadence_phrase_arbitration(local, deferred);
    CHECK(conflict.kind == ionian_cadence_phrase_interpretation_kind::
        local_close_with_deferred_resolution_conflict);
    CHECK(conflict.local_deceptive_close_grounded);
    CHECK(conflict.cross_part_continuation_grounded);
    CHECK(conflict.deferred_authentic_resolution_grounded);
    CHECK(conflict.evidence_conflict_preserved);
    CHECK(!conflict.phrase_role_established);
    CHECK(!conflict.cadence_class_established);
    CHECK(close_enough(conflict.confidence, 0.78));

    // Local closure plus continuation without a later authentic arrival is a
    // distinct conflict and must not be upgraded to deferred resolution.
    const auto continuing = continuation(false, 0.76, 0.0);
    const auto continuation_conflict =
        infer_ionian_cadence_phrase_arbitration(local, continuing);
    CHECK(continuation_conflict.kind == ionian_cadence_phrase_interpretation_kind::
        local_close_with_continuation_conflict);
    CHECK(continuation_conflict.evidence_conflict_preserved);
    CHECK(!continuation_conflict.deferred_authentic_resolution_grounded);
    CHECK(close_enough(continuation_conflict.confidence, 0.76));

    // With no independently grounded local close, the larger-span reading stays
    // available but still remains only a candidate.
    const auto only_deferred =
        infer_ionian_cadence_phrase_arbitration(unresolved_local(), deferred);
    CHECK(only_deferred.kind == ionian_cadence_phrase_interpretation_kind::
        deferred_authentic_resolution_candidate);
    CHECK(!only_deferred.evidence_conflict_preserved);
    CHECK(!only_deferred.cadence_class_established);

    // With no continuation evidence, the independently grounded local close is
    // preserved without pretending the final cadence class has been settled.
    ionian_deferred_authentic_resolution unresolved_deferred;
    unresolved_deferred.diversion_time = at(200);
    unresolved_deferred.diversion_source_degree = 5;
    unresolved_deferred.diversion_target_degree = 6;
    unresolved_deferred.diversion_diatonic = true;
    unresolved_deferred.diversion_root_motion_reliable = true;
    const auto only_local =
        infer_ionian_cadence_phrase_arbitration(local, unresolved_deferred);
    CHECK(only_local.kind == ionian_cadence_phrase_interpretation_kind::
        local_deceptive_close_candidate);
    CHECK(!only_local.evidence_conflict_preserved);
    CHECK(!only_local.cadence_class_established);

    // Two interpretations can only be arbitrated if they refer to the same
    // exact V -> VI arrival.
    bool mismatched_arrival_rejected = false;
    try {
        auto stale = deferred;
        stale.diversion_time = at(201);
        (void)infer_ionian_cadence_phrase_arbitration(local, stale);
    } catch (const std::invalid_argument&) {
        mismatched_arrival_rejected = true;
    }
    CHECK(mismatched_arrival_rejected);

    return 0;
}
