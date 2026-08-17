#include "components/spc/spc_sample_lineage_verification.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::spc;

    constexpr std::size_t frames = 64;
    std::array<float, frames + 8> upstream{};
    std::array<std::int16_t, frames> game{};

    for (std::size_t i = 0; i < upstream.size(); ++i) {
        const double phase = static_cast<double>(i) * 0.19;
        upstream[i] = static_cast<float>(std::sin(phase) * 0.5);
    }

    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {1, 2};
    candidate.upstream_identity = {3, 4};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {
        upstream.data(), upstream.size(), 48000.0, 20000.0
    };
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 2.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.0;
    candidate.coordinate_map.preparation_chain_exact = true;

    // Build the synthetic game-prepared waveform from the same candidate
    // transform, then quantize it as a decoded historical sample would be.
    for (std::size_t i = 0; i < frames; ++i) {
        const auto sample = reconstruct_spc_upstream_candidate_sample(candidate, static_cast<double>(i));
        assert(sample.valid);
        game[i] = static_cast<std::int16_t>(std::lround(sample.sample));
    }

    const auto metrics = measure_spc_sample_lineage(candidate, game.data(), game.size());
    assert(metrics.valid);
    assert(metrics.compared_frames == frames);
    assert(metrics.correlation > 0.999999);
    assert(metrics.normalized_rmse < 0.001);
    assert(metrics.gain_ratio > 0.999 && metrics.gain_ratio < 1.001);

    assert(validate_exact_spc_sample_lineage_candidate(
        candidate, game.data(), game.size()));
    assert(candidate.identity_validation_passed);
    assert(may_use_spc_sample_restoration_automatically(candidate));

    // Similar-looking audio cannot promote a source when provenance says it is
    // merely a library/preset relative rather than the exact pre-BRR source.
    auto relative = candidate;
    relative.relation = spc_sample_lineage_relation::same_preset_or_library_variant;
    relative.identity_validation_passed = true;
    assert(!validate_exact_spc_sample_lineage_candidate(
        relative, game.data(), game.size()));
    assert(!relative.identity_validation_passed);

    // A wrong preparation transform fails the waveform validation even when the
    // source identity fields are populated.
    auto wrong_map = candidate;
    wrong_map.identity_validation_passed = false;
    wrong_map.coordinate_map.upstream_origin += 1.0;
    assert(!validate_exact_spc_sample_lineage_candidate(
        wrong_map, game.data(), game.size()));
    assert(!wrong_map.identity_validation_passed);

    // A short coincidence is not enough evidence for automatic replacement.
    auto short_candidate = candidate;
    short_candidate.identity_validation_passed = false;
    assert(!validate_exact_spc_sample_lineage_candidate(
        short_candidate, game.data(), 8));

    return 0;
}
