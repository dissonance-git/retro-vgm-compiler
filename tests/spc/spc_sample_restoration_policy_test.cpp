#include "components/spc/spc_sample_restoration.h"
#include "components/spc/spc_upstream_candidate_ranking.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::spc;

    std::array<float, 16> source{{
        0.00f, 0.05f, 0.18f, 0.42f,
        0.73f, 0.91f, 0.68f, 0.31f,
       -0.12f,-0.47f,-0.71f,-0.52f,
       -0.21f, 0.08f, 0.29f, 0.11f,
    }};

    spc_sample_restoration_candidate exact;
    exact.game_brr_identity = {1, 2};
    exact.upstream_identity = {3, 4};
    exact.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    exact.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    exact.basis = spc_sample_restoration_basis::exact_upstream_pcm;
    exact.upstream.mono_pcm = source.data();
    exact.upstream.frame_count = source.size();
    exact.upstream.sample_rate_hz = 32000.0;
    exact.upstream.game_pcm_units_per_source_unit = 32767.0;
    exact.coordinate_map.upstream_frames_per_game_sample = 1.0;
    exact.coordinate_map.preparation_chain_exact = true;
    exact.identity_validation_passed = true;

    assert(classify_spc_sample_restoration(exact)
        == spc_sample_restoration_permission::source_supported_automatic);
    assert(may_use_spc_sample_restoration_automatically(exact));

    auto inverse = exact;
    inverse.basis = spc_sample_restoration_basis::deterministic_inverse_estimate;
    assert(classify_spc_sample_restoration(inverse)
        == spc_sample_restoration_permission::reversible_experiment);
    assert(!may_use_spc_sample_restoration_automatically(inverse));

    auto generative = exact;
    generative.basis = spc_sample_restoration_basis::generative_bandwidth_extension;
    assert(classify_spc_sample_restoration(generative)
        == spc_sample_restoration_permission::reversible_experiment);
    assert(!may_use_spc_sample_restoration_automatically(generative));

    // A weak lineage does not become admissible merely because the bytes are
    // real PCM and the candidate passes a same-instrument listening test.
    auto preset_relative = exact;
    preset_relative.relation = spc_sample_lineage_relation::same_preset_or_library_variant;
    assert(classify_spc_sample_restoration(preset_relative)
        == spc_sample_restoration_permission::reference_only);

    // Exact upstream bytes without the preparation map are still insufficient:
    // the game may have trimmed, resampled, looped, normalized, or otherwise
    // prepared them before BRR encoding.
    auto unmapped = exact;
    unmapped.coordinate_map.preparation_chain_exact = false;
    assert(classify_spc_sample_restoration(unmapped)
        == spc_sample_restoration_permission::reference_only);

    // Literature-backed robust matching belongs one rung below lineage. A
    // candidate can survive an affine level/DC change and become a strong
    // discovery hit without that score granting historical-source authority.
    std::array<float, source.size()> transformed_game{};
    for (std::size_t i = 0; i < source.size(); ++i)
        transformed_game[i] = source[i] * 1234.0f + 17.0f;

    const auto strong = score_spc_upstream_candidate(
        transformed_game.data(), transformed_game.size(), exact);
    assert(strong.valid);
    assert(strong.frames_compared == transformed_game.size());
    assert(strong.zero_mean_correlation > 0.999);
    assert(std::abs(strong.fitted_bias - 17.0) < 0.1);
    assert(strong_spc_upstream_candidate(strong));

    // The matching score is deliberately not consulted by the permission gate.
    // Remove exact lineage and automatic restoration still fails closed even
    // though the waveform is an excellent signal-level match.
    auto discovery_only = exact;
    discovery_only.relation = spc_sample_lineage_relation::same_preset_or_library_variant;
    const auto discovery_score = score_spc_upstream_candidate(
        transformed_game.data(), transformed_game.size(), discovery_only);
    assert(strong_spc_upstream_candidate(discovery_score));
    assert(classify_spc_sample_restoration(discovery_only)
        == spc_sample_restoration_permission::reference_only);

    std::array<float, source.size()> unrelated{};
    for (std::size_t i = 0; i < unrelated.size(); ++i)
        unrelated[i] = (i & 1u) == 0u ? 1.0f : -1.0f;
    const auto weak = score_spc_upstream_candidate(
        unrelated.data(), unrelated.size(), exact);
    assert(weak.valid);
    assert(!strong_spc_upstream_candidate(weak, 0.99));

    return 0;
}