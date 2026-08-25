#include "model/part_role_gesture_descriptor.h"

#include <cassert>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_span window(std::int64_t begin, std::int64_t end) {
    return time_span{
        time_coordinate{time_domain::source, begin, 1000, 0},
        time_coordinate{time_domain::source, end, 1000, 0},
    };
}

part_gesture_observation gesture(
    node_id source_node,
    node_id part_id,
    std::int64_t tick,
    std::optional<double> pitch,
    std::string basis = "shared-relative-pitch") {
    return {
        source_node,
        part_id,
        time_coordinate{time_domain::source, tick, 1000, 0},
        pitch,
        std::move(basis),
        "log2_frequency_ratio_octaves",
    };
}

bool contains_role(
    const part_role_window_result& result,
    musical_part_role role) {
    for (const auto& candidate : result.candidates) {
        if (candidate.role == role)
            return true;
    }
    return false;
}

} // namespace

int main() {
    // Same four-event gesture returns transposed and at 2x absolute timing.
    // Motif normalization should recover the recurrence while register remains
    // a neutral source-relative coordinate, not a role label.
    const std::vector<part_gesture_observation> repeated{
        gesture(1, 10, 0, 4.00),
        gesture(2, 10, 100, 4.25),
        gesture(3, 10, 200, 4.50),
        gesture(4, 10, 400, 4.25),
        gesture(5, 10, 1000, 5.00),
        gesture(6, 10, 1200, 5.25),
        gesture(7, 10, 1400, 5.50),
        gesture(8, 10, 1800, 5.25),
    };

    part_motif_discovery_policy policy;
    policy.min_events = 4;
    policy.max_events = 4;
    policy.min_identity_confidence = 0.85;
    policy.require_pitch_comparison = true;

    const auto descriptor = make_part_role_window_descriptor_from_gestures(
        repeated,
        10,
        window(0, 2000),
        policy);
    assert(descriptor.onset_count == 8);
    assert(descriptor.register_coordinate.has_value());
    assert(descriptor.register_basis == "shared-relative-pitch");
    assert(descriptor.structural_motif_prominence.has_value());
    assert(std::fabs(descriptor.structural_motif_prominence->value - 1.0) < 1e-12);
    assert(std::fabs(descriptor.structural_motif_prominence->confidence - 1.0) < 1e-12);
    assert(descriptor.rhythmic_repetition.has_value());
    assert(std::fabs(descriptor.rhythmic_repetition->value - 1.0) < 1e-12);
    assert(std::fabs(descriptor.rhythmic_repetition->confidence - 1.0) < 1e-12);

    // The long gap and repeated-motif boundary agree at tick 1000. Because
    // timing and motif analysis are independent origins, the phrase layer can
    // legitimately corroborate structural motif evidence for this same part.
    assert(descriptor.phrase_boundary_participation.has_value());
    assert(std::fabs(descriptor.phrase_boundary_participation->value - 1.0) < 1e-12);
    assert(descriptor.phrase_boundary_participation->confidence >= 0.80);

    const double expected_register =
        (4.00 + 4.25 + 4.50 + 4.25 + 5.00 + 5.25 + 5.50 + 5.25) / 8.0;
    assert(std::fabs(*descriptor.register_coordinate - expected_register) < 1e-12);

    const auto phrased_roles = infer_part_roles_for_window(
        {descriptor},
        "source-backed-phrase-role-bridge-test");
    assert(contains_role(phrased_roles, musical_part_role::melodic_foreground));

    // Motif recurrence alone may suggest a phrase boundary, but it is the same
    // structural evidence under another label. Without another phrase evidence
    // domain it must not self-corroborate into a foreground role.
    const std::vector<part_gesture_observation> motif_only{
        gesture(31, 30, 0, 4.00),
        gesture(32, 30, 100, 4.25),
        gesture(33, 30, 200, 4.50),
        gesture(34, 30, 300, 4.25),
        gesture(35, 30, 400, 5.00),
        gesture(36, 30, 500, 5.25),
        gesture(37, 30, 600, 5.50),
        gesture(38, 30, 700, 5.25),
    };
    const auto motif_only_descriptor = make_part_role_window_descriptor_from_gestures(
        motif_only,
        30,
        window(0, 800),
        policy);
    assert(motif_only_descriptor.structural_motif_prominence.has_value());
    assert(!motif_only_descriptor.phrase_boundary_participation.has_value());
    const auto motif_only_roles = infer_part_roles_for_window(
        {motif_only_descriptor},
        "motif-self-corroboration-control");
    assert(!contains_role(motif_only_roles, musical_part_role::melodic_foreground));

    // A rhythm-only recurrence remains useful but inherits the motif layer's
    // 0.55 identity ceiling. It must not impersonate structural motif evidence.
    std::vector<part_gesture_observation> rhythm_only = repeated;
    for (auto& observation : rhythm_only) {
        observation.log2_pitch_coordinate.reset();
        observation.pitch_basis.clear();
    }
    policy.require_pitch_comparison = false;
    const auto rhythm_descriptor = make_part_role_window_descriptor_from_gestures(
        rhythm_only,
        10,
        window(0, 2000),
        policy);
    assert(!rhythm_descriptor.register_coordinate.has_value());
    assert(rhythm_descriptor.register_basis.empty());
    assert(!rhythm_descriptor.structural_motif_prominence.has_value());
    assert(rhythm_descriptor.rhythmic_repetition.has_value());
    assert(rhythm_descriptor.rhythmic_repetition->value <=
        rhythm_only_motif_identity_ceiling + 1e-12);
    assert(!rhythm_descriptor.phrase_boundary_participation.has_value());

    // Performed trajectory shape can ground structural motif identity even when
    // onset pitch is unavailable. Here it also coincides with independent timing
    // phrase evidence, so the descriptor can climb one rung further without any
    // manually injected role signal.
    auto performed_only = rhythm_only;
    for (auto& observation : performed_only) {
        observation.performance_shape = part_gesture_performance_shape{
            pitch_motion_articulation_kind::glide_candidate,
            3.0,
            3.0,
            0,
            0.95,
        };
    }
    const auto performed_descriptor = make_part_role_window_descriptor_from_gestures(
        performed_only,
        10,
        window(0, 2000),
        policy);
    assert(!performed_descriptor.register_coordinate.has_value());
    assert(performed_descriptor.structural_motif_prominence.has_value());
    assert(performed_descriptor.structural_motif_prominence->value >
        rhythm_only_motif_identity_ceiling);
    assert(std::fabs(performed_descriptor.structural_motif_prominence->confidence - 1.0) < 1e-12);
    assert(performed_descriptor.rhythmic_repetition.has_value());
    assert(performed_descriptor.phrase_boundary_participation.has_value());
    assert(performed_descriptor.phrase_boundary_participation->confidence >= 0.80);

    const auto performed_roles = infer_part_roles_for_window(
        {performed_descriptor},
        "performed-shape-phrase-role-bridge-test");
    assert(contains_role(performed_roles, musical_part_role::melodic_foreground));

    // A short passage with no two non-overlapping motif windows does not get a
    // repetition signal merely because it has several onsets.
    const std::vector<part_gesture_observation> short_line{
        gesture(20, 20, 0, 3.0),
        gesture(21, 20, 100, 3.1),
        gesture(22, 20, 250, 3.4),
        gesture(23, 20, 500, 3.2),
    };
    policy.min_events = 3;
    policy.max_events = 4;
    const auto short_descriptor = make_part_role_window_descriptor_from_gestures(
        short_line,
        20,
        window(0, 600),
        policy);
    assert(short_descriptor.onset_count == 4);
    assert(!short_descriptor.structural_motif_prominence.has_value());
    assert(!short_descriptor.rhythmic_repetition.has_value());
    assert(!short_descriptor.phrase_boundary_participation.has_value());

    // Mixed native pitch bases may still contribute onsets, but they do not
    // produce one register center through an invented coordinate conversion.
    auto mixed = repeated;
    mixed[4].pitch_basis = "different-native-basis";
    const auto mixed_descriptor = make_part_role_window_descriptor_from_gestures(
        mixed,
        10,
        window(0, 2000),
        policy);
    assert(mixed_descriptor.onset_count == 8);
    assert(!mixed_descriptor.register_coordinate.has_value());
    assert(mixed_descriptor.register_basis.empty());

    return 0;
}