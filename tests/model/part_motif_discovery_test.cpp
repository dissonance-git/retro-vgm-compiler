#include "model/part_motif_discovery.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

part_gesture_observation gesture(
    node_id node,
    node_id part,
    std::int64_t tick,
    std::optional<double> pitch,
    const char* basis = "synthetic") {
    return {
        node,
        part,
        {time_domain::source, tick, 0, 0},
        pitch,
        basis,
    };
}

std::vector<part_gesture_observation> repeated_fixture(bool include_pitch = true) {
    const node_id part = 42;
    const std::optional<double> p0 = include_pitch ? std::optional<double>{0.0} : std::nullopt;
    const std::optional<double> p1 = include_pitch ? std::optional<double>{2.0 / 12.0} : std::nullopt;
    const std::optional<double> p2 = include_pitch ? std::optional<double>{4.0 / 12.0} : std::nullopt;
    const std::optional<double> p3 = include_pitch ? std::optional<double>{1.0 / 12.0} : std::nullopt;
    const std::optional<double> q0 = include_pitch ? std::optional<double>{1.0} : std::nullopt;
    const std::optional<double> q1 = include_pitch ? std::optional<double>{1.0 + 2.0 / 12.0} : std::nullopt;
    const std::optional<double> q2 = include_pitch ? std::optional<double>{1.0 + 4.0 / 12.0} : std::nullopt;
    const std::optional<double> q3 = include_pitch ? std::optional<double>{1.0 + 1.0 / 12.0} : std::nullopt;
    const char* basis = include_pitch ? "synthetic" : "";
    return {
        gesture(1, part, 0, p0, basis),
        gesture(2, part, 100, p1, basis),
        gesture(3, part, 200, p2, basis),
        gesture(4, part, 400, p3, basis),
        gesture(5, part, 1000, q0, basis),
        gesture(6, part, 1200, q1, basis),
        gesture(7, part, 1400, q2, basis),
        gesture(8, part, 1800, q3, basis),
    };
}

} // namespace

int main() {
    part_motif_discovery_policy policy;
    policy.min_events = 4;
    policy.max_events = 4;
    policy.min_identity_confidence = 0.99;
    policy.require_pitch_comparison = true;

    const auto repeated = discover_repeated_part_motifs(repeated_fixture(), policy);
    assert(repeated.size() == 1);
    assert(repeated[0].first.start_index == 0);
    assert(repeated[0].second.start_index == 4);
    assert(repeated[0].first.event_count == 4);
    assert(repeated[0].similarity.pitch_comparable);
    assert(repeated[0].similarity.identity_confidence > 0.99);

    auto changed = repeated_fixture();
    changed[5].log2_pitch_coordinate = 1.0 - 5.0 / 12.0;
    changed[6].log2_pitch_coordinate = 1.0 - 1.0 / 12.0;
    changed[7].log2_pitch_coordinate = 1.0 - 7.0 / 12.0;
    const auto rejected = discover_repeated_part_motifs(changed, policy);
    assert(rejected.empty());

    // Rhythm-only repetition can be surfaced explicitly at a lower threshold,
    // but it remains capped below strong motif identity.
    part_motif_discovery_policy rhythm_policy = policy;
    rhythm_policy.require_pitch_comparison = false;
    rhythm_policy.min_identity_confidence = 0.50;
    const auto rhythm_only = discover_repeated_part_motifs(
        repeated_fixture(false),
        rhythm_policy);
    assert(rhythm_only.size() == 1);
    assert(!rhythm_only[0].similarity.pitch_comparable);
    assert(rhythm_only[0].similarity.identity_confidence == rhythm_only_motif_identity_ceiling);

    rhythm_policy.min_identity_confidence = 0.75;
    const auto rhythm_cannot_be_strong = discover_repeated_part_motifs(
        repeated_fixture(false),
        rhythm_policy);
    assert(rhythm_cannot_be_strong.empty());

    return 0;
}
