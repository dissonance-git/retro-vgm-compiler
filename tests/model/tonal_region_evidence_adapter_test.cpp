#include "model/tonal_region_evidence_adapter.h"

#include <cmath>
#include <stdexcept>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

equal_temperament_model tuning() {
    equal_temperament_model result;
    result.divisions_per_octave = 12;
    result.reference_frequency_hz = 440.0;
    result.reference_step = 69;
    result.confidence = 1.0;
    result.source = "test 12-TET";
    return result;
}

tonal_center_hypothesis center(
    std::int64_t pitch_class,
    std::int64_t start,
    std::int64_t end) {
    tonal_center_hypothesis result;
    result.center_octave_class = equal_temperament_step_octave_class(tuning(), pitch_class);
    result.region = time_span{at(start), at(end)};
    result.independent_support_groups = 3;
    result.independent_support_origins = 3;
    result.cross_origin_grounded = true;
    result.confidence = 0.82;
    return result;
}

pitch_class_collection_profile g_major_collection() {
    pitch_class_collection_profile profile;
    profile.region = time_span{at(1050), at(1950)};
    profile.tuning = tuning();
    profile.pitch_role = musical_pitch_role::performed;
    profile.scope = pitch_class_collection_scope::structural_hypothesis;
    profile.projection_coverage = 0.95;
    profile.confidence = 0.86;
    profile.source = "structurally filtered G-major-region pitch collection";
    for (std::int64_t pitch_class : {7, 9, 11, 0, 2, 4, 6})
        profile.salience[static_cast<std::size_t>(pitch_class)] = 1.0;
    return profile;
}
} // namespace

int main() {
    const auto source = center(0, 0, 1000);
    auto target = center(7, 1000, 2000);

    tonal_center_evidence recurrence;
    recurrence.kind = tonal_center_evidence_kind::recurrence;
    recurrence.origin = tonal_center_evidence_origin::pitch_distribution;
    recurrence.center_octave_class = target.center_octave_class;
    recurrence.confidence = 0.88;
    recurrence.dependency_group = "g-center-recurrence";
    recurrence.status = evidence_status::hypothesis;
    recurrence.source = "repeated G-center pitch-distribution support";
    target.supporting_evidence.push_back(recurrence);

    phrase_boundary_consensus boundary;
    boundary.representative = at(1000);
    boundary.alignment_span = time_span{at(995), at(1005)};
    boundary.confidence = 0.84;
    boundary.cross_part_grounded = true;

    const auto partition = make_phrase_partition_region_evidence(
        boundary,
        source,
        target,
        "phrase-boundary:1000");
    CHECK(partition.kind == tonal_region_relation_evidence_kind::phrase_partition);
    CHECK(partition.origin == tonal_region_relation_evidence_origin::phrase_structure);
    CHECK(close_enough(partition.confidence, 0.84));

    cadential_arrival_hypothesis arrival;
    arrival.arrival_time = at(1200);
    arrival.confidence = 0.71;
    arrival.cross_part_phrase_grounded = true;
    arrival.harmonic_root_motion_reliable = true;

    const auto arrival_evidence = make_structural_arrival_region_evidence(
        arrival,
        target,
        "arrival:1200");
    CHECK(arrival_evidence.kind == tonal_region_relation_evidence_kind::structural_arrival);
    CHECK(arrival_evidence.origin == tonal_region_relation_evidence_origin::harmony);
    CHECK(close_enough(arrival_evidence.confidence, 0.71));

    const auto persistence = make_center_persistence_region_evidence(target);
    CHECK(persistence.kind == tonal_region_relation_evidence_kind::center_persistence);
    CHECK(persistence.origin == tonal_region_relation_evidence_origin::pitch_collection);
    CHECK(persistence.dependency_group == "g-center-recurrence");
    CHECK(close_enough(persistence.confidence, 0.82));

    const auto collection_evidence = make_structural_pitch_collection_region_evidence(
        g_major_collection(),
        target,
        "structural-collection:g");
    CHECK(collection_evidence.kind == tonal_region_relation_evidence_kind::structural_pitch_collection);
    CHECK(collection_evidence.origin == tonal_region_relation_evidence_origin::pitch_collection);
    CHECK(close_enough(collection_evidence.confidence, 0.82));

    const auto relation = infer_tonal_region_relation(
        source,
        target,
        {partition, arrival_evidence, persistence, collection_evidence});
    CHECK(relation.kind == tonal_region_relation_kind::modulation_candidate);
    CHECK(relation.independent_support_groups == 4);
    CHECK(relation.independent_support_origins == 3);

    // The weakest required clue, here the 0.71 structural arrival, bounds the
    // entire modulation candidate. Stronger partition/persistence/collection
    // evidence cannot launder it upward.
    CHECK(close_enough(relation.confidence, 0.71));
    CHECK(!relation.modulation_established);

    // A surface pitch histogram is useful descriptive evidence but cannot be
    // repackaged as structural tonal-region support.
    bool surface_rejected = false;
    try {
        auto surface = g_major_collection();
        surface.scope = pitch_class_collection_scope::surface_performance;
        (void)make_structural_pitch_collection_region_evidence(
            surface,
            target,
            "surface:g");
    } catch (const std::invalid_argument&) {
        surface_rejected = true;
    }
    CHECK(surface_rejected);

    // A single-part, non-authored phrase boundary cannot create a global tonal
    // partition merely because its confidence happens to be numerically high.
    bool weak_partition_rejected = false;
    try {
        auto weak_boundary = boundary;
        weak_boundary.cross_part_grounded = false;
        weak_boundary.authored_grounded = false;
        weak_boundary.confidence = 0.99;
        (void)make_phrase_partition_region_evidence(
            weak_boundary,
            source,
            target,
            "weak-boundary:1000");
    } catch (const std::invalid_argument&) {
        weak_partition_rejected = true;
    }
    CHECK(weak_partition_rejected);

    bool misaligned_partition_rejected = false;
    try {
        auto wrong_boundary = boundary;
        wrong_boundary.representative = at(900);
        (void)make_phrase_partition_region_evidence(
            wrong_boundary,
            source,
            target,
            "wrong-boundary:900");
    } catch (const std::invalid_argument&) {
        misaligned_partition_rejected = true;
    }
    CHECK(misaligned_partition_rejected);

    bool outside_arrival_rejected = false;
    try {
        auto outside = arrival;
        outside.arrival_time = at(2200);
        (void)make_structural_arrival_region_evidence(
            outside,
            target,
            "outside-arrival:2200");
    } catch (const std::invalid_argument&) {
        outside_arrival_rejected = true;
    }
    CHECK(outside_arrival_rejected);

    bool missing_persistence_rejected = false;
    try {
        auto no_recurrence = target;
        no_recurrence.supporting_evidence.clear();
        (void)make_center_persistence_region_evidence(no_recurrence);
    } catch (const std::invalid_argument&) {
        missing_persistence_rejected = true;
    }
    CHECK(missing_persistence_rejected);

    return 0;
}
