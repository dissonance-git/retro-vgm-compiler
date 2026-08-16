#include "model/counterpoint_motion_profile.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

std::vector<part_gesture_observation> line(
    node_id part_id,
    const char* basis,
    std::vector<double> pitches) {
    std::vector<part_gesture_observation> result;
    for (std::size_t index = 0; index < pitches.size(); ++index) {
        result.push_back({
            static_cast<node_id>(100 + part_id * 10 + index),
            part_id,
            time_coordinate{
                time_domain::source,
                static_cast<std::int64_t>(index * 100),
                0,
                0,
            },
            pitches[index],
            basis,
            "log2_frequency_ratio_octaves",
        });
    }
    return result;
}

} // namespace

int main() {
    const auto upper = line(
        1,
        "upper-native-basis",
        {0.0, 1.0 / 12.0, 2.0 / 12.0, 3.0 / 12.0});
    const auto lower = line(
        2,
        "lower-native-basis",
        {7.0 / 12.0, 6.0 / 12.0, 5.0 / 12.0, 5.0 / 12.0});

    // Each line has valid relative pitch motion even though their native pitch
    // bases differ. Contrary/oblique motion is therefore available while the
    // absolute vertical interval between the parts remains unresolved.
    const auto relative = make_counterpoint_motion_profile(upper, lower, 0.88);
    CHECK(relative.first_part_id == 1);
    CHECK(relative.second_part_id == 2);
    CHECK(relative.contrary_motion_count == 2);
    CHECK(relative.oblique_motion_count == 1);
    CHECK(relative.similar_motion_count == 0);
    CHECK(relative.stationary_motion_count == 0);
    CHECK(!relative.vertical_intervals_comparable);
    CHECK(!relative.vertical_interval_octaves.has_value());

    // Once both lines have earned a shared absolute pitch basis, vertical
    // intervals can be reported without changing the contrapuntal-motion facts.
    const auto lower_shared = line(
        2,
        "upper-native-basis",
        {7.0 / 12.0, 6.0 / 12.0, 5.0 / 12.0, 5.0 / 12.0});
    const auto absolute = make_counterpoint_motion_profile(upper, lower_shared, 0.88);
    CHECK(absolute.contrary_motion_count == 2);
    CHECK(absolute.oblique_motion_count == 1);
    CHECK(absolute.vertical_intervals_comparable);
    CHECK(absolute.vertical_interval_octaves.has_value());
    CHECK(absolute.vertical_interval_octaves->size() == 4);

    return 0;
}
