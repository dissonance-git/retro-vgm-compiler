#include "model/voice_leading_hypothesis.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

tertian_triad_hypothesis chord(
    std::int64_t tick,
    std::vector<std::int64_t> steps,
    std::vector<node_id> parts,
    double confidence = 0.90) {
    tertian_triad_hypothesis result;
    result.confidence = confidence;
    result.projection.tuning.divisions_per_octave = 12;
    result.projection.nearest_steps = std::move(steps);
    result.projection.source_verticality.part_ids = std::move(parts);
    result.projection.source_verticality.observation_time = {
        time_domain::source,
        tick,
        0,
        0,
    };
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    // Persistent part identity outranks nearest-pitch matching. Parts 1 and 2
    // cross in register, so a purely geometric assignment would under-report
    // the actual line motion.
    const auto first = chord(0, {60, 64, 67}, {1, 2, 3}, 0.94);
    const auto crossed = chord(100, {62, 63, 69}, {2, 1, 3}, 0.91);
    const auto grounded = infer_voice_leading(first, crossed);

    CHECK(grounded.motions.size() == 3);
    CHECK(grounded.identity_preserved_voices == 3);
    CHECK(grounded.all_correspondence_identity_grounded);
    CHECK(!grounded.fallback_assignment_ambiguous);
    CHECK(grounded.total_absolute_motion_semitones == 7);
    CHECK(grounded.upward_voices == 2);
    CHECK(grounded.downward_voices == 1);
    CHECK(grounded.stationary_voices == 0);
    CHECK(close_enough(grounded.confidence, 0.91));

    bool saw_part_1 = false;
    bool saw_part_2 = false;
    bool saw_part_3 = false;
    for (const auto& motion : grounded.motions) {
        if (motion.first_part_id == 1) {
            CHECK(motion.second_part_id == 1);
            CHECK(motion.semitone_motion == 3);
            saw_part_1 = true;
        } else if (motion.first_part_id == 2) {
            CHECK(motion.second_part_id == 2);
            CHECK(motion.semitone_motion == -2);
            saw_part_2 = true;
        } else if (motion.first_part_id == 3) {
            CHECK(motion.second_part_id == 3);
            CHECK(motion.semitone_motion == 2);
            saw_part_3 = true;
        }
    }
    CHECK(saw_part_1 && saw_part_2 && saw_part_3);

    // When no persistent identities survive, the system may still propose a
    // minimum-motion correspondence, but that inference is explicitly capped.
    const auto unrelated_parts = chord(100, {60, 65, 69}, {4, 5, 6}, 0.95);
    const auto inferred = infer_voice_leading(first, unrelated_parts);
    CHECK(inferred.identity_preserved_voices == 0);
    CHECK(!inferred.all_correspondence_identity_grounded);
    CHECK(!inferred.fallback_assignment_ambiguous);
    CHECK(inferred.total_absolute_motion_semitones == 3);
    CHECK(close_enough(inferred.confidence, inferred_voice_correspondence_ceiling));

    return 0;
}
