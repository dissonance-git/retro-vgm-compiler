#include "model/harmonic_pitch_class_collection_adapter.h"

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

tertian_triad_hypothesis major_triad(
    std::int64_t root,
    std::int64_t tick,
    double confidence = 0.88,
    musical_pitch_role role = musical_pitch_role::performed) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = positive_mod(root, 12);
    chord.quality = tertian_triad_quality::major;
    chord.inversion = triad_inversion::root_position;
    chord.confidence = confidence;
    chord.root_ambiguous = false;
    for (std::int64_t offset : triad_template(tertian_triad_quality::major))
        chord.pitch_classes.push_back(positive_mod(root + offset, 12));
    chord.projection.tuning = tuning();
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.role = role;
    chord.projection.confidence = confidence;
    return chord;
}

tonal_center_hypothesis grounded_c_center() {
    tonal_center_hypothesis center;
    center.center_octave_class = equal_temperament_step_octave_class(tuning(), 0);
    center.region = time_span{at(0), at(1000)};
    center.independent_support_groups = 3;
    center.independent_support_origins = 3;
    center.cross_origin_grounded = true;
    center.confidence = 0.82;
    return center;
}
} // namespace

int main() {
    const std::vector<tertian_triad_hypothesis> c_f_g = {
        major_triad(0, 100),
        major_triad(5, 400),
        major_triad(7, 700),
    };

    const auto structural = make_structural_pitch_class_collection_from_triads(
        time_span{at(0), at(1000)},
        c_f_g,
        "C-F-G structural harmonic field");
    CHECK(structural.scope == pitch_class_collection_scope::structural_hypothesis);
    CHECK(structural.pitch_role == musical_pitch_role::performed);
    CHECK(close_enough(structural.projection_coverage, 1.0));
    CHECK(close_enough(structural.confidence, 0.88));

    // C, F and G major jointly expose all seven white-note structural classes.
    for (std::int64_t pitch_class : {0, 2, 4, 5, 7, 9, 11})
        CHECK(structural.salience[static_cast<std::size_t>(pitch_class)] > 0.0);
    for (std::int64_t pitch_class : {1, 3, 6, 8, 10})
        CHECK(close_enough(structural.salience[static_cast<std::size_t>(pitch_class)], 0.0));

    // This is the first fully structural path into the diatonic key gate:
    // chord hypotheses -> structural collection -> grounded center -> key class.
    const auto key = infer_tonal_key_class_hypothesis(
        grounded_c_center(),
        structural);
    CHECK(key.key_class_resolved);
    CHECK(key.mode.has_value());
    CHECK(*key.mode == diatonic_mode::ionian);
    CHECK(key.center_pitch_class == 0);
    CHECK(close_enough(key.confidence, 0.82));
    CHECK(!key.enharmonic_spelling_named);
    CHECK(!key.tonal_function_named);

    // A weak chord cannot contribute a diagnostic pitch class and then vanish
    // from the collection's confidence accounting.
    bool weak_chord_rejected = false;
    try {
        auto weak = c_f_g;
        weak.back().confidence = 0.69;
        weak.back().projection.confidence = 0.69;
        (void)make_structural_pitch_class_collection_from_triads(
            time_span{at(0), at(1000)},
            weak,
            "weak diagnostic dominant chord");
    } catch (const std::invalid_argument&) {
        weak_chord_rejected = true;
    }
    CHECK(weak_chord_rejected);

    bool mixed_role_rejected = false;
    try {
        auto mixed = c_f_g;
        mixed.back().projection.source_verticality.role = musical_pitch_role::heard;
        (void)make_structural_pitch_class_collection_from_triads(
            time_span{at(0), at(1000)},
            mixed,
            "mixed performed/heard chord field");
    } catch (const std::invalid_argument&) {
        mixed_role_rejected = true;
    }
    CHECK(mixed_role_rejected);

    bool mixed_tuning_rejected = false;
    try {
        auto mixed = c_f_g;
        mixed.back().projection.tuning.reference_frequency_hz = 442.0;
        (void)make_structural_pitch_class_collection_from_triads(
            time_span{at(0), at(1000)},
            mixed,
            "mixed tuning chord field");
    } catch (const std::invalid_argument&) {
        mixed_tuning_rejected = true;
    }
    CHECK(mixed_tuning_rejected);

    bool outside_region_rejected = false;
    try {
        auto outside = c_f_g;
        outside.back().projection.source_verticality.observation_time = at(1000);
        (void)make_structural_pitch_class_collection_from_triads(
            time_span{at(0), at(1000)},
            outside,
            "out-of-region chord field");
    } catch (const std::invalid_argument&) {
        outside_region_rejected = true;
    }
    CHECK(outside_region_rejected);

    return 0;
}
