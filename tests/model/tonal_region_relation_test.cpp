#include "model/tonal_region_relation.h"

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
    std::int64_t end,
    double confidence = 0.82,
    bool grounded = true) {
    tonal_center_hypothesis result;
    result.center_octave_class = equal_temperament_step_octave_class(tuning(), pitch_class);
    result.region = time_span{at(start), at(end)};
    result.independent_support_groups = grounded ? 3u : 1u;
    result.independent_support_origins = grounded ? 3u : 1u;
    result.cross_origin_grounded = grounded;
    result.confidence = confidence;
    return result;
}

tonal_region_relation_evidence relation_evidence(
    tonal_region_relation_evidence_kind kind,
    tonal_region_relation_evidence_origin origin,
    double confidence,
    const char* dependency) {
    tonal_region_relation_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.confidence = confidence;
    result.dependency_group = dependency;
    result.source = "test relation evidence";
    return result;
}

std::vector<tonal_region_relation_evidence> modulation_support() {
    return {
        relation_evidence(
            tonal_region_relation_evidence_kind::phrase_partition,
            tonal_region_relation_evidence_origin::phrase_structure,
            0.91,
            "phrase-boundary:1000"),
        relation_evidence(
            tonal_region_relation_evidence_kind::structural_arrival,
            tonal_region_relation_evidence_origin::harmony,
            0.89,
            "arrival:1000"),
        relation_evidence(
            tonal_region_relation_evidence_kind::center_persistence,
            tonal_region_relation_evidence_origin::harmony,
            0.87,
            "center-persistence:g"),
        relation_evidence(
            tonal_region_relation_evidence_kind::structural_pitch_collection,
            tonal_region_relation_evidence_origin::pitch_collection,
            0.85,
            "collection:g"),
    };
}
} // namespace

int main() {
    const auto c_first = center(0, 0, 1000);
    const auto c_second = center(0, 1000, 2000);
    const auto g_second = center(7, 1000, 2000);

    // Rearticulating a grounded center in a new peer region is not a modulation.
    const auto retained = infer_tonal_region_relation(c_first, c_second, {});
    CHECK(retained.topology == tonal_region_topology::sequential);
    CHECK(retained.kind == tonal_region_relation_kind::retained_center);
    CHECK(retained.centers_equivalent);
    CHECK(close_enough(retained.confidence, tonal_region_retained_center_ceiling));

    // A changed center by itself is only contrast. No amount of naming pressure
    // may turn this into modulation without independent structural evidence.
    const auto contrast = infer_tonal_region_relation(c_first, g_second, {});
    CHECK(contrast.kind == tonal_region_relation_kind::contrasting_center);
    CHECK(!contrast.centers_equivalent);
    CHECK(close_enough(contrast.confidence, tonal_region_contrast_ceiling));
    CHECK(!contrast.modulation_established);

    // A foreign center nested inside a larger C-centered region can become a
    // tonicization candidate once independent arrival and structural-collection
    // evidence support the local G region.
    const auto c_parent = center(0, 0, 2000);
    const auto g_nested = center(7, 500, 900);
    const auto tonicization = infer_tonal_region_relation(
        c_parent,
        g_nested,
        {
            relation_evidence(
                tonal_region_relation_evidence_kind::structural_arrival,
                tonal_region_relation_evidence_origin::phrase_structure,
                0.88,
                "nested-arrival:500"),
            relation_evidence(
                tonal_region_relation_evidence_kind::structural_pitch_collection,
                tonal_region_relation_evidence_origin::pitch_collection,
                0.84,
                "nested-collection:g"),
        });
    CHECK(tonicization.topology == tonal_region_topology::target_nested_in_source);
    CHECK(tonicization.kind == tonal_region_relation_kind::tonicization_candidate);
    CHECK(tonicization.independent_support_groups == 2);
    CHECK(tonicization.independent_support_origins == 2);
    CHECK(close_enough(
        tonicization.confidence,
        tonal_region_tonicization_candidate_ceiling));
    CHECK(!tonicization.tonicization_established);

    // A sequential peer region needs partition, arrival, persistence and
    // structurally grounded pitch content before it can even be called a
    // modulation candidate.
    const auto modulation = infer_tonal_region_relation(
        c_first,
        g_second,
        modulation_support());
    CHECK(modulation.kind == tonal_region_relation_kind::modulation_candidate);
    CHECK(modulation.independent_support_groups == 4);
    CHECK(modulation.independent_support_origins == 3);
    CHECK(close_enough(
        modulation.confidence,
        tonal_region_modulation_candidate_ceiling));
    CHECK(!modulation.modulation_established);

    // Remove persistence and the same center change falls back to mere contrast.
    auto no_persistence = modulation_support();
    no_persistence.erase(no_persistence.begin() + 2);
    const auto transient = infer_tonal_region_relation(
        c_first,
        g_second,
        no_persistence);
    CHECK(transient.kind == tonal_region_relation_kind::contrasting_center);
    CHECK(close_enough(transient.confidence, tonal_region_contrast_ceiling));

    // Four labels derived from one witness are still one witness.
    auto collapsed_support = modulation_support();
    for (auto& item : collapsed_support)
        item.dependency_group = "same-underlying-witness";
    const auto collapsed = infer_tonal_region_relation(
        c_first,
        g_second,
        collapsed_support);
    CHECK(collapsed.independent_support_groups == 1);
    CHECK(collapsed.kind == tonal_region_relation_kind::contrasting_center);

    // A return is a three-region statement: earlier C, intervening G, then C
    // again, plus independent re-entry/form/arrival evidence.
    const auto c_return = center(0, 2000, 3000);
    const auto returned = infer_tonal_region_relation(
        g_second,
        c_return,
        {
            relation_evidence(
                tonal_region_relation_evidence_kind::return_reentry,
                tonal_region_relation_evidence_origin::form,
                0.90,
                "return-reentry:c"),
            relation_evidence(
                tonal_region_relation_evidence_kind::form_recurrence,
                tonal_region_relation_evidence_origin::form,
                0.88,
                "formal-recurrence:a"),
            relation_evidence(
                tonal_region_relation_evidence_kind::structural_arrival,
                tonal_region_relation_evidence_origin::phrase_structure,
                0.86,
                "return-arrival:2000"),
        },
        std::nullopt,
        c_first);
    CHECK(returned.kind == tonal_region_relation_kind::return_candidate);
    CHECK(returned.return_reference_supplied);
    CHECK(returned.target_matches_return_reference);
    CHECK(close_enough(returned.confidence, tonal_region_return_candidate_ceiling));

    // Strong explicit counterevidence cannot be averaged away by four positive
    // transformations of the same musical transition.
    auto conflicted_support = modulation_support();
    auto contradiction = relation_evidence(
        tonal_region_relation_evidence_kind::contradiction,
        tonal_region_relation_evidence_origin::external_annotation,
        0.95,
        "independent-counterevidence");
    contradiction.supports_relation = false;
    conflicted_support.push_back(contradiction);
    const auto conflicted = infer_tonal_region_relation(
        c_first,
        g_second,
        conflicted_support);
    CHECK(conflicted.kind == tonal_region_relation_kind::modulation_candidate);
    CHECK(conflicted.strong_counterevidence);
    CHECK(close_enough(conflicted.confidence, tonal_region_strong_conflict_ceiling));

    // A resolved key from a different center cannot be loaned to this region as
    // structural support.
    bool mismatched_key_rejected = false;
    try {
        tonal_key_class_hypothesis wrong_key;
        wrong_key.region = time_span{at(1100), at(1900)};
        wrong_key.center_octave_class = c_first.center_octave_class;
        wrong_key.center_pitch_class = 0;
        wrong_key.mode = diatonic_mode::ionian;
        wrong_key.key_class_resolved = true;
        wrong_key.confidence = 0.80;
        (void)infer_tonal_region_relation(
            c_first,
            g_second,
            modulation_support(),
            wrong_key);
    } catch (const std::invalid_argument&) {
        mismatched_key_rejected = true;
    }
    CHECK(mismatched_key_rejected);

    // The reference for a return must actually precede the intervening region.
    bool late_reference_rejected = false;
    try {
        const auto late_reference = center(0, 1500, 2500);
        (void)infer_tonal_region_relation(
            g_second,
            c_return,
            {},
            std::nullopt,
            late_reference);
    } catch (const std::invalid_argument&) {
        late_reference_rejected = true;
    }
    CHECK(late_reference_rejected);

    // Materialization validates support nodes before changing the graph.
    auto invalid_materialization = modulation;
    invalid_materialization.supporting_evidence.front().source_node = 999;
    musical_execution_graph graph;
    bool unknown_support_rejected = false;
    try {
        (void)add_tonal_region_relation_hypothesis(graph, invalid_materialization);
    } catch (const std::invalid_argument&) {
        unknown_support_rejected = true;
    }
    CHECK(unknown_support_rejected);
    CHECK(graph.nodes().empty());
    CHECK(graph.edges().empty());

    return 0;
}
