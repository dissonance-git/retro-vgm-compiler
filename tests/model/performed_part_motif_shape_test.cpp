#include "../../model/part_motif_profile.h"

#include <cassert>
#include <cmath>
#include <optional>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 1000, 0};
}

part_gesture_performance_shape shape(
    pitch_motion_articulation_kind kind,
    double range,
    double net,
    std::size_t changes) {
    return {kind, range, net, changes, 0.96};
}

part_gesture_observation gesture(
    node_id source,
    std::int64_t tick,
    double pitch,
    std::optional<part_gesture_performance_shape> performance_shape) {
    part_gesture_observation result{
        source,
        900,
        at(tick),
        pitch,
        "performed_frequency",
        "log2_frequency_ratio_octaves",
        evidence_status::derived,
        0.96,
    };
    result.performance_shape = std::move(performance_shape);
    return result;
}
}

int main() {
    const std::vector<part_gesture_observation> first{
        gesture(1, 0, 0.0, shape(pitch_motion_articulation_kind::steady_pitch, 0.01, 0.0, 0)),
        gesture(2, 100, 1.0, shape(pitch_motion_articulation_kind::glide_candidate, 4.0, 4.0, 0)),
        gesture(3, 200, 0.0, shape(pitch_motion_articulation_kind::periodic_modulation_candidate, 1.2, 0.1, 3)),
    };
    const auto same_profile = make_part_motif_profile(first);
    assert(same_profile.performance_shapes.has_value());
    assert(same_profile.performance_shapes->size() == 3);

    auto changed = first;
    changed[1].performance_shape = shape(pitch_motion_articulation_kind::steady_pitch, 0.02, 0.0, 0);
    changed[2].performance_shape = shape(pitch_motion_articulation_kind::glide_candidate, 5.0, -5.0, 0);
    const auto changed_profile = make_part_motif_profile(changed);

    const auto same = compare_part_motif_profiles(same_profile, same_profile);
    const auto different_shape = compare_part_motif_profiles(same_profile, changed_profile);
    assert(same.pitch_comparable);
    assert(same.performance_shape_comparable);
    assert(same.performance_shape_similarity.has_value());
    assert(std::fabs(*same.performance_shape_similarity - 1.0) < 1e-12);
    assert(std::fabs(same.combined_similarity - 1.0) < 1e-12);
    assert(different_shape.pitch_comparable);
    assert(different_shape.performance_shape_comparable);
    assert(different_shape.performance_shape_similarity.has_value());
    assert(*different_shape.performance_shape_similarity < 1.0);
    assert(different_shape.combined_similarity < same.combined_similarity);

    auto unresolved = first;
    unresolved[1].performance_shape.reset();
    const auto unresolved_profile = make_part_motif_profile(unresolved);
    assert(!unresolved_profile.performance_shapes.has_value());
    const auto missing_shape = compare_part_motif_profiles(same_profile, unresolved_profile);
    assert(missing_shape.pitch_comparable);
    assert(!missing_shape.performance_shape_comparable);
    assert(!missing_shape.performance_shape_similarity.has_value());

    return 0;
}
