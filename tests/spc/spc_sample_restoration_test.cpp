#include "components/spc/spc_sample_restoration.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::spc;

    std::array<float, 16> upstream_pcm{};
    for (std::size_t i = 0; i < upstream_pcm.size(); ++i)
        upstream_pcm[i] = static_cast<float>(i) / 16.0f;

    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {1, 2};
    candidate.upstream_identity = {3, 4};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {upstream_pcm.data(), upstream_pcm.size(), 48000.0};
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 2.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.5;
    candidate.coordinate_map.loop_present = true;
    candidate.coordinate_map.game_loop_start = 4.0;
    candidate.coordinate_map.upstream_loop_start = 8.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;

    assert(classify_spc_sample_restoration(candidate)
        == spc_sample_restoration_permission::source_supported_automatic);
    assert(may_use_spc_sample_restoration_automatically(candidate));
    assert(std::abs(candidate.coordinate_map.map_position(4.0) - 8.0) < 1.0e-12);
    assert(std::abs(candidate.coordinate_map.map_loop_start() - 8.0) < 1.0e-12);
    assert(spc_upstream_position_available(candidate, 4.0));
    assert(!spc_upstream_position_available(candidate, 20.0));

    // Knowing a source waveform is not enough. If the historical game-sample
    // preparation cannot be replayed exactly, automatic substitution is denied.
    auto missing_preparation = candidate;
    missing_preparation.coordinate_map.preparation_chain_exact = false;
    assert(classify_spc_sample_restoration(missing_preparation)
        == spc_sample_restoration_permission::reference_only);

    // Same-library/preset similarity is not exact lineage, even when it sounds
    // plausible and happens to have a convenient coordinate map.
    auto related_variant = candidate;
    related_variant.relation = spc_sample_lineage_relation::same_preset_or_library_variant;
    assert(classify_spc_sample_restoration(related_variant)
        == spc_sample_restoration_permission::reference_only);

    // Mechanically plausible or aesthetic restorations stay reversible
    // experiments. Enhanced playback must not silently upgrade them to history.
    auto mechanical = candidate;
    mechanical.evidence = spc_sample_restoration_evidence::deterministic_ceiling;
    assert(classify_spc_sample_restoration(mechanical)
        == spc_sample_restoration_permission::reversible_experiment);
    assert(!may_use_spc_sample_restoration_automatically(mechanical));

    auto aesthetic = candidate;
    aesthetic.evidence = spc_sample_restoration_evidence::aesthetic_hypothesis;
    assert(classify_spc_sample_restoration(aesthetic)
        == spc_sample_restoration_permission::reversible_experiment);

    // Even an exact source/provenance match waits for a same-instrument
    // validation result before it becomes the normal automatic Enhanced path.
    auto unvalidated = candidate;
    unvalidated.identity_validation_passed = false;
    assert(classify_spc_sample_restoration(unvalidated)
        == spc_sample_restoration_permission::reversible_experiment);

    // Missing content identity is unknown evidence, not a filename-based guess.
    auto no_identity = candidate;
    no_identity.upstream_identity = {};
    assert(classify_spc_sample_restoration(no_identity)
        == spc_sample_restoration_permission::reference_only);

    return 0;
}
