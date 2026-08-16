#include "model/pitch_class_collection_adapter.h"

#include <cmath>
#include <stdexcept>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second, double epsilon = 1e-9) {
    return std::fabs(first - second) < epsilon;
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

double frequency_for_step(std::int64_t step, double cents_offset = 0.0) {
    return 440.0 * std::pow(
        2.0,
        (static_cast<double>(step - 69) + cents_offset / 100.0) / 12.0);
}

absolute_musical_pitch_observation pitch(
    node_id source,
    node_id part,
    std::int64_t start,
    std::int64_t end,
    std::int64_t step,
    double confidence = 0.90,
    double cents_offset = 0.0,
    musical_pitch_role role = musical_pitch_role::performed) {
    absolute_musical_pitch_observation observation;
    observation.source_node = source;
    observation.part_id = part;
    observation.active = time_span{at(start), at(end)};
    observation.frequency_hz = frequency_for_step(step, cents_offset);
    observation.role = role;
    observation.status = evidence_status::hypothesis;
    observation.confidence = confidence;
    observation.source = "test performed pitch";
    return observation;
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
    std::vector<absolute_musical_pitch_observation> white_notes;
    const std::int64_t steps[] = {60, 62, 64, 65, 67, 69, 71};
    for (std::size_t index = 0; index < 7; ++index) {
        white_notes.push_back(pitch(
            static_cast<node_id>(index + 1),
            static_cast<node_id>(100 + index),
            static_cast<std::int64_t>(index * 100),
            static_cast<std::int64_t>((index + 1) * 100),
            steps[index]));
    }

    const auto surface = make_surface_pitch_class_collection(
        time_span{at(0), at(700)},
        tuning(),
        white_notes,
        "seven-note performed surface");
    CHECK(surface.scope == pitch_class_collection_scope::surface_performance);
    CHECK(surface.pitch_role == musical_pitch_role::performed);
    CHECK(close_enough(surface.projection_coverage, 1.0));
    CHECK(close_enough(surface.confidence, 0.90));
    CHECK(surface.salience[0] > 0.0);
    CHECK(surface.salience[2] > 0.0);
    CHECK(surface.salience[4] > 0.0);
    CHECK(surface.salience[5] > 0.0);
    CHECK(surface.salience[7] > 0.0);
    CHECK(surface.salience[9] > 0.0);
    CHECK(surface.salience[11] > 0.0);

    // Surface evidence can rank a theory-scoped candidate, but cannot itself
    // become structural key evidence.
    const auto unresolved_surface_key = infer_tonal_key_class_hypothesis(
        grounded_c_center(),
        surface);
    CHECK(!unresolved_surface_key.key_class_resolved);
    CHECK(!unresolved_surface_key.mode.has_value());
    CHECK(unresolved_surface_key.alternatives.front().mode == diatonic_mode::ionian);

    // Two 50-cent deviations lie outside the 35-cent projection tolerance.
    // They reduce coverage explicitly instead of disappearing from the evidence
    // accounting while the remaining notes pretend the fit was complete.
    auto detuned = white_notes;
    detuned[5] = pitch(6, 105, 500, 600, 69, 0.90, 50.0);
    detuned[6] = pitch(7, 106, 600, 700, 71, 0.90, -50.0);
    const auto partial = make_surface_pitch_class_collection(
        time_span{at(0), at(700)},
        tuning(),
        detuned,
        "partially off-grid performed surface");
    CHECK(close_enough(partial.projection_coverage, 5.0 / 7.0));
    CHECK(partial.projection_coverage < diatonic_key_min_projection_coverage);
    CHECK(partial.confidence < surface.confidence);

    // Programmed, performed, and heard pitch are distinct evidence domains and
    // cannot be mixed into one collection without an explicit adapter.
    bool mixed_role_rejected = false;
    try {
        auto mixed = white_notes;
        mixed[1].role = musical_pitch_role::heard;
        (void)make_surface_pitch_class_collection(
            time_span{at(0), at(700)},
            tuning(),
            mixed,
            "mixed-role surface");
    } catch (const std::invalid_argument&) {
        mixed_role_rejected = true;
    }
    CHECK(mixed_role_rejected);

    // Open-ended pitch evidence is clipped to the finite requested region, not
    // treated as infinite duration.
    auto sustained = pitch(20, 200, 0, 100, 60);
    sustained.active.end = std::nullopt;
    const auto clipped = make_surface_pitch_class_collection(
        time_span{at(25), at(75)},
        tuning(),
        {sustained},
        "clipped sustained pitch");
    CHECK(clipped.salience[0] > 0.0);
    CHECK(close_enough(clipped.projection_coverage, 1.0));

    return 0;
}
