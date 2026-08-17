#include "components/spc/spc_sample_restoration.h"

#include <array>
#include <cassert>

int main() {
    using namespace gameaudio::spc;

    std::array<float, 8> source{{0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.3f, 0.2f, 0.1f}};

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

    return 0;
}
