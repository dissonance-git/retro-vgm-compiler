#include "model/motif_composer_grammar_bridge.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

part_motif_profile profile(
    node_id part,
    const char* native_basis,
    std::vector<double> rhythm,
    std::vector<double> intervals,
    std::vector<std::int8_t> contour) {
    part_motif_profile result;
    result.part_id = part;
    result.normalized_inter_onset_intervals = std::move(rhythm);
    result.interval_octaves = std::move(intervals);
    result.pitch_contour = std::move(contour);
    result.pitch_basis = native_basis;
    result.interval_semantics = "log2_frequency_ratio_octaves";
    return result;
}

} // namespace

int main() {
    // Same musical relation, deliberately different native representations.
    const auto genesis = profile(
        1,
        "genesis_ym2612_relative_frequency_code",
        {1.0, 1.0, 2.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1});
    const auto spc = profile(
        2,
        "spc_brr_runtime_version:99",
        {1.0, 1.0, 2.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1});

    const auto similarity = compare_part_motif_profiles(genesis, spc);
    assert(similarity.pitch_comparable);
    assert(close_enough(similarity.identity_confidence, 1.0));

    const motif_grammar_context genesis_context{
        "genesis-soundtrack",
        "work-A",
        composer_representation_kind::synthesis_runtime,
        "blind-genesis-motif-analysis",
    };
    const motif_grammar_context spc_context{
        "snes-soundtrack",
        "work-B",
        composer_representation_kind::synthesis_runtime,
        "blind-spc-motif-analysis",
    };

    // No candidate identity is supplied to this bridge. It only projects an
    // already-established musical relation into two provenance-bearing work
    // observations. Candidate identity enters at aggregation time below.
    const auto observations = motif_similarity_as_grammar_observations(
        genesis_context,
        spc_context,
        similarity,
        creative_attribution_role::composer,
        "transposed motif relation survives Genesis-to-SPC representation change");

    assert(observations.first.soundtrack_id == "genesis-soundtrack");
    assert(observations.second.soundtrack_id == "snes-soundtrack");
    assert(observations.first.dimension == composer_grammar_dimension::motif_development);
    assert(observations.second.dimension == composer_grammar_dimension::motif_development);
    assert(close_enough(observations.first.confidence, 1.0));
    assert(close_enough(observations.second.confidence, 1.0));

    const auto rule = make_composer_grammar_rule(
        "candidate-only-entered-here",
        creative_attribution_role::composer,
        "cross-platform-motif-cell",
        0.96,
        {observations.first, observations.second});

    assert(rule.cross_work_grounded);
    assert(rule.cross_soundtrack_grounded);
    assert(rule.independent_soundtracks == 2);
    assert(rule.grounding_support_observations == 2);
    assert(close_enough(rule.independent_support_ceiling, 1.0));
    assert(close_enough(rule.confidence, 0.96));

    // A rhythm-only relation stays below the grammar grounding threshold even
    // if it appears in two differently named soundtracks.
    part_motif_similarity weak;
    weak.rhythm_similarity = 1.0;
    weak.combined_similarity = 1.0;
    weak.identity_confidence = rhythm_only_motif_identity_ceiling;
    weak.pitch_comparable = false;

    const auto weak_observations = motif_similarity_as_grammar_observations(
        genesis_context,
        spc_context,
        weak,
        creative_attribution_role::composer,
        "rhythm-only echo");
    const auto weak_rule = make_composer_grammar_rule(
        "candidate-only-entered-here",
        creative_attribution_role::composer,
        "rhythm-only-echo",
        0.96,
        {weak_observations.first, weak_observations.second});

    assert(weak_rule.supporting_observations == 2);
    assert(weak_rule.grounding_support_observations == 0);
    assert(!weak_rule.cross_soundtrack_grounded);
    assert(close_enough(weak_rule.confidence, composer_grammar_weak_support_ceiling));

    return 0;
}
