#include "model/part_motif_attribution_bridge.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

part_motif_profile profile(
    node_id part_id,
    std::int64_t step,
    double base_pitch,
    const std::vector<double>& pitch_offsets,
    std::string pitch_basis) {
    std::vector<part_gesture_observation> observations;
    for (std::size_t i = 0; i < pitch_offsets.size(); ++i) {
        observations.push_back({
            static_cast<node_id>(1000 + part_id * 10 + i),
            part_id,
            {time_domain::device, static_cast<std::int64_t>(i) * step, 32000, 0},
            base_pitch + pitch_offsets[i],
            pitch_basis,
            "log2_frequency_ratio_octaves",
            evidence_status::derived,
            1.0,
        });
    }
    return make_part_motif_profile(observations);
}

} // namespace

int main() {
    const auto query_up = profile(1, 100, 4.0, {0.0, 0.25, 0.50}, "query-a");
    const auto query_down = profile(2, 100, 5.0, {0.50, 0.25, 0.0}, "query-b");

    // Different absolute pitch basis and twice the tempo scale, but identical
    // relative interval/rhythm geometry.
    const auto control_up = profile(3, 200, 7.0, {0.0, 0.25, 0.50}, "control-x");
    const auto control_down = profile(4, 200, 3.0, {0.50, 0.25, 0.0}, "control-y");

    const auto exact_geometry = compare_part_motif_profile_sets(
        {query_up, query_down},
        {control_down, control_up});
    assert(exact_geometry.query_profile_count == 2);
    assert(exact_geometry.control_profile_count == 2);
    assert(exact_geometry.matched_pair_count == 2);
    assert(exact_geometry.pitch_comparable_pair_count == 2);
    assert(std::fabs(exact_geometry.matched_coverage - 1.0) < 1e-12);
    assert(std::fabs(exact_geometry.similarity - 1.0) < 1e-12);

    // One matching part cannot explain two query parts. The unmatched part is a
    // zero in the cue-level denominator rather than being silently discarded.
    const auto one_part_only = compare_part_motif_profile_sets(
        {query_up, query_down},
        {control_up});
    assert(one_part_only.matched_pair_count == 1);
    assert(std::fabs(one_part_only.matched_coverage - 0.5) < 1e-12);
    assert(one_part_only.similarity <= 0.5 + 1e-12);

    const auto match = make_part_motif_composer_control_match(
        "sonic3-unknown",
        "Calibration Composer",
        "control-track",
        "control-soundtrack",
        "control-work",
        "Super NES",
        "spc-driver-family-a",
        0.96,
        exact_geometry,
        "direct-exact-track-credit");
    assert(match.role == creative_attribution_role::composer);
    assert(match.representation == composer_representation_kind::synthesis_runtime);
    assert(match.dimension == composer_grammar_dimension::melody);
    assert(match.status == evidence_status::hypothesis);
    assert(std::fabs(match.match_strength - 1.0) < 1e-12);
    assert(std::fabs(match.confidence - 0.96) < 1e-12);
    assert(match.source == "blind-part-motif:synthesis-runtime");

    return 0;
}
