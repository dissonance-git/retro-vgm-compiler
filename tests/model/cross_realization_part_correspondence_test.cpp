#include "model/cross_realization_part_correspondence.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

part_gesture_observation gesture(
    node_id source,
    node_id part,
    std::int64_t tick,
    double frequency_hz,
    double confidence = 0.95) {
    part_gesture_observation result;
    result.source_node = source;
    result.part_id = part;
    result.onset = at(tick);
    result.log2_pitch_coordinate = std::log2(frequency_hz);
    result.pitch_basis = "absolute_performed_frequency_hz";
    result.interval_semantics = "log2_frequency_ratio_octaves";
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    return result;
}

std::vector<part_gesture_observation> line(
    node_id first_source,
    node_id part,
    std::int64_t lag,
    double multiplier = 1.0) {
    return {
        gesture(first_source + 0, part, 0 + lag, 440.0 * multiplier),
        gesture(first_source + 1, part, 100 + lag, 550.0 * multiplier),
        gesture(first_source + 2, part, 200 + lag, 660.0 * multiplier),
        gesture(first_source + 3, part, 300 + lag, 550.0 * multiplier),
    };
}

} // namespace

int main() {
    const auto fm = line(100, 10, 0, 1.0);

    {
        const auto psg = line(200, 20, 0, 1.0);
        const auto relation = infer_cross_realization_part_correspondence(
            fm, psg, "YM2612", "SN76489_tone");
        CHECK(relation.kind ==
            cross_realization_correspondence_kind::synchronous_unison_doubling_candidate);
        CHECK(relation.timing_correspondence_grounded);
        CHECK(relation.pitch_correspondence_grounded);
        CHECK(std::fabs(relation.median_pitch_offset_octaves) < 1e-9);
        CHECK(std::fabs(relation.normalized_onset_lag) < 1e-9);
        CHECK(relation.confidence <= cross_realization_structural_only_ceiling);
    }

    {
        const auto psg = line(300, 30, 0, 2.0);
        const auto relation = infer_cross_realization_part_correspondence(
            fm, psg, "YM2612", "SN76489_tone");
        CHECK(relation.kind ==
            cross_realization_correspondence_kind::synchronous_octave_doubling_candidate);
        CHECK(std::fabs(relation.median_pitch_offset_octaves - 1.0) < 1e-9);
    }

    {
        // A quarter-IOI delay is too large for synchronous doubling but is
        // stable enough across the whole gesture to become a delayed-shadow
        // candidate. This is the Kasatani-style shape the Sonic lane needs to
        // distinguish from an independent PSG melody.
        const auto psg = line(400, 40, 25, 1.0);
        const auto relation = infer_cross_realization_part_correspondence(
            fm, psg, "YM2612", "SN76489_tone");
        CHECK(relation.kind ==
            cross_realization_correspondence_kind::delayed_shadow_candidate);
        CHECK(std::fabs(relation.normalized_onset_lag - 0.25) < 1e-9);
        CHECK(relation.normalized_lag_dispersion < 1e-9);
    }

    {
        // Same hardware overlap is not enough. A genuinely different gesture
        // should not be forced into a doubling relation merely because both
        // lines use pitched synthesis at the same time.
        const std::vector<part_gesture_observation> psg = {
            gesture(500, 50, 0, 880.0),
            gesture(501, 50, 70, 440.0),
            gesture(502, 50, 210, 990.0),
            gesture(503, 50, 330, 495.0),
        };
        const auto relation = infer_cross_realization_part_correspondence(
            fm, psg, "YM2612", "SN76489_tone");
        CHECK(relation.kind ==
                  cross_realization_correspondence_kind::independent_line_candidate ||
              relation.kind ==
                  cross_realization_correspondence_kind::weak_relation);
        CHECK(relation.kind !=
            cross_realization_correspondence_kind::synchronous_unison_doubling_candidate);
        CHECK(relation.kind !=
            cross_realization_correspondence_kind::delayed_shadow_candidate);
    }

    {
        bool rejected = false;
        try {
            (void)infer_cross_realization_part_correspondence(
                fm, line(600, 60, 0), "SN76489_tone", "SN76489_tone");
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        CHECK(rejected);
    }

    return 0;
}
