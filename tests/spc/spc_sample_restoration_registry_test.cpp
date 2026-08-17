#include "components/spc/spc_sample_restoration_registry.h"

#include <array>
#include <cassert>

namespace {

gameaudio::spc::spc_sample_restoration_candidate make_exact_candidate(
    const gameaudio::spc::spc_sample_content_identity game_id,
    const gameaudio::spc::spc_sample_content_identity upstream_id,
    const float* pcm,
    std::size_t frames) {
    using namespace gameaudio::spc;
    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = game_id;
    candidate.upstream_identity = upstream_id;
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.basis = spc_sample_restoration_basis::exact_upstream_pcm;
    candidate.upstream.mono_pcm = pcm;
    candidate.upstream.frame_count = frames;
    candidate.upstream.sample_rate_hz = 44100.0;
    candidate.upstream.game_pcm_units_per_source_unit = 32767.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;
    return candidate;
}

} // namespace

int main() {
    using namespace gameaudio::spc;

    std::array<float, 8> piano{{0.0f, 0.2f, 0.4f, 0.6f, 0.4f, 0.2f, 0.0f, -0.1f}};
    std::array<float, 8> brass{{0.0f, 0.3f, 0.5f, 0.2f, -0.2f, -0.4f, -0.1f, 0.0f}};

    spc_sample_restoration_registry<2> registry;
    const auto first = make_exact_candidate({1, 1}, {10, 10}, piano.data(), piano.size());
    const auto second = make_exact_candidate({2, 2}, {20, 20}, brass.data(), brass.size());

    assert(registry.insert_automatic(first));
    assert(registry.size() == 1);
    assert(registry.find({1, 1}) != nullptr);
    assert(registry.find({1, 1})->upstream.mono_pcm == piano.data());

    // No insertion-order winner is allowed for two claims about the same BRR
    // object. The corpus/adjudication layer must resolve the conflict first.
    auto conflicting = second;
    conflicting.game_brr_identity = {1, 1};
    assert(!registry.insert_automatic(conflicting));
    assert(registry.size() == 1);

    // An inferred waveform cannot enter the automatic runtime registry.
    auto inferred = second;
    inferred.basis = spc_sample_restoration_basis::generative_bandwidth_extension;
    assert(!registry.insert_automatic(inferred));

    assert(registry.insert_automatic(second));
    assert(registry.size() == 2);

    const auto third = make_exact_candidate({3, 3}, {30, 30}, piano.data(), piano.size());
    assert(!registry.insert_automatic(third));
    assert(registry.size() == registry.capacity());

    registry.clear();
    assert(registry.size() == 0);
    assert(registry.find({1, 1}) == nullptr);

    return 0;
}
