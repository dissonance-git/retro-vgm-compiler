#include "model/tonal_center_evidence_adapter.h"

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

tertian_triad_hypothesis c_major_at(
    std::int64_t tick,
    double confidence,
    triad_inversion inversion = triad_inversion::root_position) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = 0;
    chord.quality = tertian_triad_quality::major;
    chord.inversion = inversion;
    chord.confidence = confidence;
    chord.root_ambiguous = false;
    chord.pitch_classes = {0, 4, 7};
    chord.projection.tuning.divisions_per_octave = 12;
    chord.projection.tuning.reference_frequency_hz = 440.0;
    chord.projection.tuning.reference_step = 69;
    chord.projection.tuning.confidence = 1.0;
    chord.projection.tuning.source = "test 12-TET";
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.part_ids = {7, 8, 9};
    chord.projection.nearest_steps = {60, 64, 67};
    chord.projection.confidence = confidence;
    return chord;
}
} // namespace

int main() {
    const auto first = c_major_at(0, 0.91);
    const auto second = c_major_at(100, 0.89);

    harmonic_transition_hypothesis transition;
    transition.first_time = at(0);
    transition.second_time = at(100);
    transition.first_root_pitch_class = 0;
    transition.second_root_pitch_class = 0;
    transition.directed_root_motion_semitones = 0;
    transition.root_interval_class = 0;
    transition.root_motion_reliable = true;
    transition.confidence = 0.90;

    const auto harmonic = make_harmonic_stability_center_evidence(
        first,
        second,
        transition,
        "harmony:0-100");
    CHECK(harmonic.kind == tonal_center_evidence_kind::harmonic_stability);
    CHECK(harmonic.origin == tonal_center_evidence_origin::harmony);
    CHECK(close_enough(harmonic.confidence, 0.89));

    bass_harmony_interaction_hypothesis bass;
    bass.kind = bass_harmony_interaction_kind::harmonic_identity_retained;
    bass.bass_part_id = 7;
    bass.bass_identity_grounded = true;
    bass.confidence = 0.87;

    const auto bass_support = make_bass_support_center_evidence(
        second,
        bass,
        "bass-trajectory:7");
    CHECK(bass_support.kind == tonal_center_evidence_kind::bass_support);
    CHECK(bass_support.origin == tonal_center_evidence_origin::bass_structure);
    CHECK(close_enough(bass_support.confidence, 0.87));
    CHECK(close_enough(
        bass_support.center_octave_class,
        harmonic.center_octave_class));

    cadential_arrival_hypothesis arrival;
    arrival.arrival_time = at(100);
    arrival.confidence = 0.84;

    const auto structural_arrival = make_structural_arrival_center_evidence(
        arrival,
        second,
        "phrase-arrival:100");
    CHECK(structural_arrival.kind == tonal_center_evidence_kind::structural_arrival);
    CHECK(structural_arrival.origin == tonal_center_evidence_origin::phrase_structure);
    CHECK(close_enough(structural_arrival.confidence, 0.84));

    const time_span region{at(0), at(200)};
    const auto center = infer_tonal_center_hypothesis(
        harmonic.center_octave_class,
        region,
        {harmonic, bass_support, structural_arrival});
    CHECK(center.independent_support_groups == 3);
    CHECK(center.independent_support_origins == 3);
    CHECK(center.cross_origin_grounded);
    CHECK(close_enough(center.confidence, tonal_center_three_group_ceiling));
    CHECK(!center.key_named);
    CHECK(!center.mode_named);
    CHECK(!center.tonal_function_named);

    // If harmonic and bass claims are merely two derivatives of one witness,
    // the caller can put them in one dependency group and they collapse to one
    // independent vote.
    const auto harmonic_shared = make_harmonic_stability_center_evidence(
        first,
        second,
        transition,
        "verticality-family:0-100");
    const auto bass_shared = make_bass_support_center_evidence(
        second,
        bass,
        "verticality-family:0-100");
    const auto collapsed = infer_tonal_center_hypothesis(
        harmonic.center_octave_class,
        region,
        {harmonic_shared, bass_shared, structural_arrival});
    CHECK(collapsed.independent_support_groups == 2);
    CHECK(collapsed.independent_support_origins == 2);
    CHECK(close_enough(collapsed.confidence, tonal_center_two_group_ceiling));

    // A first-inversion chord cannot be repackaged as bass evidence for the
    // chord root simply because its root is otherwise unambiguous.
    bool inversion_rejected = false;
    try {
        const auto inverted = c_major_at(100, 0.89, triad_inversion::first);
        (void)make_bass_support_center_evidence(
            inverted,
            bass,
            "bass-trajectory:7");
    } catch (const std::invalid_argument&) {
        inversion_rejected = true;
    }
    CHECK(inversion_rejected);

    // Root ambiguity remains a hard barrier at this layer.
    bool ambiguous_root_rejected = false;
    try {
        auto ambiguous = second;
        ambiguous.root_ambiguous = true;
        (void)make_structural_arrival_center_evidence(
            arrival,
            ambiguous,
            "phrase-arrival:100");
    } catch (const std::invalid_argument&) {
        ambiguous_root_rejected = true;
    }
    CHECK(ambiguous_root_rejected);

    return 0;
}
