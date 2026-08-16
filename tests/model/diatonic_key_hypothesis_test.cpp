#include "model/diatonic_key_hypothesis.h"

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

equal_temperament_model tuning() {
    equal_temperament_model result;
    result.divisions_per_octave = 12;
    result.reference_frequency_hz = 440.0;
    result.reference_step = 69;
    result.confidence = 1.0;
    result.source = "test 12-TET";
    return result;
}

tonal_center_hypothesis grounded_center(std::int64_t pitch_class, double confidence = 0.82) {
    tonal_center_hypothesis center;
    center.center_octave_class = equal_temperament_step_octave_class(tuning(), pitch_class);
    center.region = time_span{at(0), at(1000)};
    center.independent_support_groups = 3;
    center.independent_support_origins = 3;
    center.cross_origin_grounded = true;
    center.confidence = confidence;
    return center;
}

pitch_class_collection_profile collection(std::initializer_list<std::int64_t> pitch_classes) {
    pitch_class_collection_profile profile;
    profile.region = time_span{at(100), at(900)};
    profile.tuning = tuning();
    profile.confidence = 0.92;
    profile.source = "test structural pitch-class collection";
    for (std::int64_t pitch_class : pitch_classes)
        profile.salience[static_cast<std::size_t>(positive_mod(pitch_class, 12))] = 1.0;
    return profile;
}
} // namespace

int main() {
    const auto white_notes = collection({0, 2, 4, 5, 7, 9, 11});

    // Full white-note collection under a grounded C center resolves Ionian.
    const auto c_ionian = infer_tonal_key_class_hypothesis(
        grounded_center(0),
        white_notes);
    CHECK(c_ionian.key_class_resolved);
    CHECK(c_ionian.mode.has_value());
    CHECK(*c_ionian.mode == diatonic_mode::ionian);
    CHECK(c_ionian.center_pitch_class == 0);
    CHECK(c_ionian.distinct_pitch_classes == 7);
    CHECK(c_ionian.separation >= diatonic_key_min_separation);
    CHECK(close_enough(c_ionian.confidence, 0.82));
    CHECK(!c_ionian.enharmonic_spelling_named);
    CHECK(!c_ionian.tonal_function_named);
    CHECK(c_ionian.alternatives.size() == 7);

    // The identical pitch collection under a grounded D center resolves Dorian,
    // proving that collection identity is not silently treated as key identity.
    const auto d_dorian = infer_tonal_key_class_hypothesis(
        grounded_center(2),
        white_notes);
    CHECK(d_dorian.key_class_resolved);
    CHECK(d_dorian.mode.has_value());
    CHECK(*d_dorian.mode == diatonic_mode::dorian);
    CHECK(d_dorian.center_pitch_class == 2);

    // A tonic triad is far too sparse to name a mode or key class.
    const auto c_triad = infer_tonal_key_class_hypothesis(
        grounded_center(0),
        collection({0, 4, 7}));
    CHECK(!c_triad.key_class_resolved);
    CHECK(!c_triad.mode.has_value());
    CHECK(c_triad.distinct_pitch_classes == 3);

    // C D E F G A is compatible with both Ionian and Mixolydian because the
    // diagnostic seventh is absent. Six pitch classes alone are not enough if
    // the actual alternatives remain tied.
    const auto missing_seventh = infer_tonal_key_class_hypothesis(
        grounded_center(0),
        collection({0, 2, 4, 5, 7, 9}));
    CHECK(!missing_seventh.key_class_resolved);
    CHECK(!missing_seventh.mode.has_value());
    CHECK(close_enough(missing_seventh.separation, 0.0));

    // Perfect scale content cannot rescue a center that was never independently
    // grounded in the first place.
    auto weak_center = grounded_center(0, tonal_center_single_support_ceiling);
    weak_center.cross_origin_grounded = false;
    weak_center.independent_support_groups = 1;
    weak_center.independent_support_origins = 1;
    const auto weak = infer_tonal_key_class_hypothesis(weak_center, white_notes);
    CHECK(!weak.key_class_resolved);
    CHECK(!weak.mode.has_value());

    // A local collection must actually live inside the tonal-center region. A
    // stale or unrelated region is not allowed to borrow the center label.
    bool outside_region_rejected = false;
    try {
        auto outside = white_notes;
        outside.region = time_span{at(900), at(1200)};
        (void)infer_tonal_key_class_hypothesis(grounded_center(0), outside);
    } catch (const std::invalid_argument&) {
        outside_region_rejected = true;
    }
    CHECK(outside_region_rejected);

    return 0;
}
