#include "model/part_motif_profile.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

part_gesture_observation gesture(
    node_id node,
    node_id part,
    std::int64_t tick,
    double pitch,
    const char* basis = "synthetic-pitch",
    std::int64_t loop_iteration = 0,
    const char* interval_semantics = "log2_frequency_ratio_octaves",
    evidence_status status = evidence_status::derived,
    double confidence = 1.0) {
    return {
        node,
        part,
        {time_domain::source, tick, 0, loop_iteration},
        pitch,
        basis,
        interval_semantics,
        status,
        confidence,
    };
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) <= tolerance;
}

} // namespace

int main() {
    const node_id part_a = 100;
    const node_id part_b = 200;

    const auto original = make_part_motif_profile({
        gesture(1, part_a, 0, 0.00),
        gesture(2, part_a, 100, 2.0 / 12.0),
        gesture(3, part_a, 200, 4.0 / 12.0),
        gesture(4, part_a, 400, 1.0 / 12.0),
    });

    // Same interval/rhythm grammar after one-octave transposition and 2x time
    // scaling should be identical in the profile space.
    const auto transformed = make_part_motif_profile({
        gesture(11, part_b, 0, 1.00),
        gesture(12, part_b, 200, 1.0 + 2.0 / 12.0),
        gesture(13, part_b, 400, 1.0 + 4.0 / 12.0),
        gesture(14, part_b, 800, 1.0 + 1.0 / 12.0),
    });

    const auto invariant = compare_part_motif_profiles(original, transformed);
    assert(invariant.pitch_comparable);
    assert(invariant.interval_similarity.has_value());
    assert(invariant.contour_similarity.has_value());
    assert(close_enough(*invariant.interval_similarity, 1.0));
    assert(close_enough(invariant.rhythm_similarity, 1.0));
    assert(close_enough(*invariant.contour_similarity, 1.0));
    assert(close_enough(invariant.combined_similarity, 1.0));
    assert(close_enough(invariant.evidence_confidence, 1.0));
    assert(close_enough(invariant.identity_confidence, 1.0));
    assert(invariant.transposition_invariant);
    assert(invariant.tempo_scale_invariant);

    // Perfect structural correspondence cannot strengthen uncertain upstream
    // evidence. This is important for FM missing-fundamental and other
    // performed-pitch hypotheses whose geometry may be exact conditional on a
    // confidence-capped interpretation.
    const auto uncertain_transformed = make_part_motif_profile({
        gesture(15, part_b, 0, 1.00, "synthetic-pitch", 0,
            "log2_frequency_ratio_octaves", evidence_status::hypothesis, 0.68),
        gesture(16, part_b, 200, 1.0 + 2.0 / 12.0, "synthetic-pitch", 0,
            "log2_frequency_ratio_octaves", evidence_status::hypothesis, 0.68),
        gesture(17, part_b, 400, 1.0 + 4.0 / 12.0, "synthetic-pitch", 0,
            "log2_frequency_ratio_octaves", evidence_status::hypothesis, 0.68),
        gesture(18, part_b, 800, 1.0 + 1.0 / 12.0, "synthetic-pitch", 0,
            "log2_frequency_ratio_octaves", evidence_status::hypothesis, 0.68),
    });
    const auto uncertain_match = compare_part_motif_profiles(
        original,
        uncertain_transformed);
    assert(uncertain_match.pitch_comparable);
    assert(close_enough(uncertain_match.combined_similarity, 1.0));
    assert(close_enough(uncertain_match.evidence_confidence, 0.68));
    assert(close_enough(uncertain_match.identity_confidence, 0.68));
    assert(uncertain_transformed.status == evidence_status::hypothesis);
    assert(close_enough(uncertain_transformed.evidence_confidence, 0.68));

    const auto different = make_part_motif_profile({
        gesture(21, 300, 0, 0.00),
        gesture(22, 300, 100, -5.0 / 12.0),
        gesture(23, 300, 350, -1.0 / 12.0),
        gesture(24, 300, 450, -7.0 / 12.0),
    });
    const auto contrast = compare_part_motif_profiles(original, different);
    assert(contrast.pitch_comparable);
    assert(contrast.interval_similarity.has_value());
    assert(contrast.contour_similarity.has_value());
    assert(contrast.combined_similarity < 0.80);
    assert(close_enough(contrast.identity_confidence, contrast.combined_similarity));
    assert(*contrast.contour_similarity < 1.0);

    // Unknown pitch cannot establish strong motif identity even when rhythm is
    // an exact match.
    std::vector<part_gesture_observation> no_pitch = {
        {31, 400, {time_domain::source, 0, 0, 0}, std::nullopt, "", ""},
        {32, 400, {time_domain::source, 100, 0, 0}, std::nullopt, "", ""},
        {33, 400, {time_domain::source, 200, 0, 0}, std::nullopt, "", ""},
        {34, 400, {time_domain::source, 400, 0, 0}, std::nullopt, "", ""},
    };
    const auto rhythm_only = make_part_motif_profile(no_pitch);
    const auto rhythm_comparison = compare_part_motif_profiles(original, rhythm_only);
    assert(!rhythm_comparison.pitch_comparable);
    assert(!rhythm_comparison.interval_similarity.has_value());
    assert(!rhythm_comparison.contour_similarity.has_value());
    assert(close_enough(rhythm_comparison.rhythm_similarity, 1.0));
    assert(close_enough(rhythm_comparison.combined_similarity, 1.0));
    assert(close_enough(
        rhythm_comparison.identity_confidence,
        rhythm_only_motif_identity_ceiling));

    // Different native pitch coordinates may still yield the same derived
    // interval semantics. This is the cross-representation bridge used for
    // Genesis frequency ratios versus SPC pitch-rate ratios.
    const auto different_native_basis = make_part_motif_profile({
        gesture(41, 500, 0, 0.00, "other-native-basis"),
        gesture(42, 500, 100, 2.0 / 12.0, "other-native-basis"),
        gesture(43, 500, 200, 4.0 / 12.0, "other-native-basis"),
        gesture(44, 500, 400, 1.0 / 12.0, "other-native-basis"),
    });
    const auto cross_representation = compare_part_motif_profiles(
        original,
        different_native_basis);
    assert(cross_representation.pitch_comparable);
    assert(close_enough(*cross_representation.interval_similarity, 1.0));
    assert(close_enough(cross_representation.identity_confidence, 1.0));

    const auto incompatible_interval_semantics = make_part_motif_profile({
        gesture(45, 501, 0, 0.00, "other-native-basis", 0, "non_frequency_pitch_units"),
        gesture(46, 501, 100, 2.0 / 12.0, "other-native-basis", 0, "non_frequency_pitch_units"),
        gesture(47, 501, 200, 4.0 / 12.0, "other-native-basis", 0, "non_frequency_pitch_units"),
        gesture(48, 501, 400, 1.0 / 12.0, "other-native-basis", 0, "non_frequency_pitch_units"),
    });
    const auto semantic_guard = compare_part_motif_profiles(
        original,
        incompatible_interval_semantics);
    assert(!semantic_guard.pitch_comparable);
    assert(!semantic_guard.interval_similarity.has_value());
    assert(!semantic_guard.contour_similarity.has_value());
    assert(semantic_guard.identity_confidence <= rhythm_only_motif_identity_ceiling);

    bool rejected_mixed_parts = false;
    try {
        (void)make_part_motif_profile({
            gesture(51, 600, 0, 0.0),
            gesture(52, 601, 100, 0.1),
            gesture(53, 600, 200, 0.2),
        });
    } catch (const std::invalid_argument&) {
        rejected_mixed_parts = true;
    }
    assert(rejected_mixed_parts);

    bool rejected_reversed_time = false;
    try {
        (void)make_part_motif_profile({
            gesture(61, 700, 0, 0.0),
            gesture(62, 700, 100, 0.1),
            gesture(63, 700, 50, 0.2),
        });
    } catch (const std::invalid_argument&) {
        rejected_reversed_time = true;
    }
    assert(rejected_reversed_time);

    bool rejected_cross_loop = false;
    try {
        (void)make_part_motif_profile({
            gesture(71, 800, 0, 0.0, "synthetic-pitch", 0),
            gesture(72, 800, 100, 0.1, "synthetic-pitch", 0),
            gesture(73, 800, 20, 0.2, "synthetic-pitch", 1),
        });
    } catch (const std::invalid_argument&) {
        rejected_cross_loop = true;
    }
    assert(rejected_cross_loop);

    return 0;
}
