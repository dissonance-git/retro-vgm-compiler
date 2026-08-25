#include "model/pitch_motion_articulation.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

std::vector<pitch_motion_sample> samples(
    node_id episode,
    const std::vector<double>& semitone_offsets) {
    std::vector<pitch_motion_sample> result;
    for (std::size_t index = 0; index < semitone_offsets.size(); ++index) {
        result.push_back({
            static_cast<node_id>(100 + index),
            episode,
            time_coordinate{
                time_domain::source,
                static_cast<std::int64_t>(index * 10),
                0,
                0,
            },
            semitone_offsets[index] / 12.0,
            "shared-log2-frequency",
            "log2_frequency_ratio_octaves",
        });
    }
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    // Oscillation inside one physical voice episode is modulation evidence, not
    // a stream of fake note retriggers.
    const auto vibrato = analyze_in_episode_pitch_motion(
        samples(1, {0.0, 0.5, -0.5, 0.5, 0.0}),
        0.92);
    CHECK(vibrato.kind == pitch_motion_articulation_kind::periodic_modulation_candidate);
    CHECK(!vibrato.rearticulation_supported);
    CHECK(vibrato.direction_changes >= 2);
    CHECK(close_enough(vibrato.net_motion_semitones, 0.0));

    // Monotonic movement across several control states is a glide candidate.
    const auto glide = analyze_in_episode_pitch_motion(
        samples(2, {0.0, 0.5, 1.0, 1.5}),
        0.90);
    CHECK(glide.kind == pitch_motion_articulation_kind::glide_candidate);
    CHECK(!glide.rearticulation_supported);
    CHECK(glide.direction_changes == 0);
    CHECK(close_enough(glide.net_motion_semitones, 1.5));

    // Even a large in-episode jump is not enough to manufacture a new note.
    const auto unresolved_jump = analyze_in_episode_pitch_motion(
        samples(3, {0.0, 7.0}),
        0.88);
    CHECK(unresolved_jump.kind ==
        pitch_motion_articulation_kind::in_episode_pitch_change_unresolved);
    CHECK(!unresolved_jump.rearticulation_supported);

    // Rearticulation becomes strong only when the bounded physical episode
    // itself ends and a distinct one begins.
    const auto rearticulation = make_rearticulation_boundary(3, 4, 0.96);
    CHECK(rearticulation.kind == pitch_motion_articulation_kind::rearticulation_boundary);
    CHECK(rearticulation.rearticulation_supported);
    CHECK(rearticulation.physical_episode_id == 3);
    CHECK(rearticulation.next_physical_episode_id == 4);
    CHECK(close_enough(rearticulation.confidence, 0.96));

    return 0;
}
