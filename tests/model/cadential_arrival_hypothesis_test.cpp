#include "model/cadential_arrival_hypothesis.h"

#include <cmath>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {
time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}
} // namespace

int main() {
    phrase_boundary_consensus boundary;
    boundary.representative = at(100);
    boundary.confidence = 0.90;
    boundary.cross_part_grounded = true;
    boundary.cross_origin_grounded = true;
    boundary.independently_grounded = true;

    harmonic_transition_hypothesis harmony;
    harmony.first_time = at(0);
    harmony.second_time = at(100);
    harmony.first_root_pitch_class = 7;
    harmony.second_root_pitch_class = 0;
    harmony.first_quality = tertian_triad_quality::major;
    harmony.second_quality = tertian_triad_quality::major;
    harmony.directed_root_motion_semitones = 5;
    harmony.root_interval_class = 5;
    harmony.common_pitch_classes = 1;
    harmony.root_motion_reliable = true;
    harmony.confidence = 0.91;

    const auto two_domain = infer_cadential_arrival(boundary, harmony);
    CHECK(two_domain.departure_time.tick == 0);
    CHECK(two_domain.arrival_time.tick == 100);
    CHECK(two_domain.cross_part_phrase_grounded);
    CHECK(close_enough(two_domain.confidence, phrase_harmony_arrival_ceiling));
    CHECK(!two_domain.voice_leading_grounded);
    CHECK(!two_domain.tonal_function_named);

    auto common_mode_boundary = boundary;
    common_mode_boundary.cross_origin_grounded = false;
    common_mode_boundary.independently_grounded = false;
    const auto common_mode = infer_cadential_arrival(common_mode_boundary, harmony);
    CHECK(!common_mode.cross_part_phrase_grounded);
    CHECK(!common_mode.tonal_function_named);

    voice_leading_hypothesis voices;
    voices.first_time = at(0);
    voices.second_time = at(100);
    voices.total_absolute_motion_semitones = 3;
    voices.all_correspondence_identity_grounded = true;
    voices.confidence = 0.88;

    const auto three_domain = infer_cadential_arrival(boundary, harmony, voices);
    CHECK(three_domain.voice_leading_grounded);
    CHECK(three_domain.total_voice_motion_semitones == 3);
    CHECK(close_enough(three_domain.confidence, identity_grounded_arrival_ceiling));
    CHECK(!three_domain.tonal_function_named);

    auto changed_quality = harmony;
    changed_quality.second_root_pitch_class = 9;
    changed_quality.second_quality = tertian_triad_quality::minor;
    changed_quality.quality_changed = true;
    changed_quality.directed_root_motion_semitones = 2;
    changed_quality.root_interval_class = 2;
    const auto changed = infer_cadential_arrival(boundary, changed_quality);
    CHECK(changed.arrival_root_pitch_class == 9);
    CHECK(changed.arrival_quality == tertian_triad_quality::minor);
    CHECK(changed.quality_changed);
    CHECK(!changed.tonal_function_named);

    harmony.root_motion_reliable = false;
    const auto ambiguous_root = infer_cadential_arrival(boundary, harmony, voices);
    CHECK(close_enough(ambiguous_root.confidence, unreliable_root_arrival_ceiling));

    return 0;
}
