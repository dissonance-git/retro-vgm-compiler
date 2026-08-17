#include "components/spc/spc_enhanced_source_policy.h"

#include <array>
#include <cassert>

int main() {
    using namespace gameaudio::spc;

    spc_enhanced_source_capabilities caps;
    assert(select_spc_enhanced_source_rung(caps)
        == spc_enhanced_source_rung::protected_reference);

    caps.high_rate_brr_reconstruction = true;
    assert(select_spc_enhanced_source_rung(caps)
        == spc_enhanced_source_rung::high_rate_brr_reconstruction);

    caps.prebrr_game_grid_available = true;
    assert(select_spc_enhanced_source_rung(caps)
        == spc_enhanced_source_rung::verified_prebrr_game_grid);
    assert(spc_enhanced_rung_removes_brr_loss(
        spc_enhanced_source_rung::verified_prebrr_game_grid));
    assert(!spc_enhanced_rung_restores_preparation_lost_bandwidth(
        spc_enhanced_source_rung::verified_prebrr_game_grid));

    std::array<float, 32> pcm{};
    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {1, 2};
    candidate.upstream_identity = {3, 4};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {pcm.data(), pcm.size(), 48000.0, 32767.0};
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.5;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;
    caps.original_source = &candidate;

    assert(select_spc_enhanced_source_rung(caps)
        == spc_enhanced_source_rung::verified_original_source);
    assert(spc_enhanced_rung_removes_brr_loss(
        spc_enhanced_source_rung::verified_original_source));
    assert(spc_enhanced_rung_restores_preparation_lost_bandwidth(
        spc_enhanced_source_rung::verified_original_source));

    // Merely finding a similar library sample cannot outrank even the prepared
    // pre-BRR grid, let alone become a direct-original source automatically.
    candidate.identity_validation_passed = false;
    assert(select_spc_enhanced_source_rung(caps)
        == spc_enhanced_source_rung::verified_prebrr_game_grid);

    return 0;
}
