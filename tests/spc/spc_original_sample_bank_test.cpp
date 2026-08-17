#include "components/spc/spc_original_sample_bank.h"

#include <array>
#include <cassert>

namespace {

gameaudio::spc::spc_sample_restoration_candidate make_candidate(
    const gameaudio::spc::spc_sample_content_identity game,
    const gameaudio::spc::spc_sample_content_identity upstream,
    const float* pcm,
    std::size_t frames) {
    using namespace gameaudio::spc;
    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = game;
    candidate.upstream_identity = upstream;
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {pcm, frames, 48000.0, 32767.0};
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 0.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.5;
    candidate.coordinate_map.loop_present = true;
    candidate.coordinate_map.game_loop_start = 8.0;
    candidate.coordinate_map.upstream_loop_start = 12.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;
    return candidate;
}

} // namespace

int main() {
    using namespace gameaudio::spc;

    std::array<float, 64> pcm{};
    for (std::size_t i = 0; i < pcm.size(); ++i)
        pcm[i] = static_cast<float>(i) / static_cast<float>(pcm.size());

    const spc_sample_content_identity game{1, 2};
    const spc_sample_content_identity upstream{3, 4};

    spc_original_sample_bank<4> bank;
    assert(bank.add(make_candidate(game, upstream, pcm.data(), pcm.size())));
    const auto* selected = bank.find_automatic(game);
    assert(selected != nullptr);
    assert(same_spc_sample_content_identity(selected->upstream_identity, upstream));

    // Duplicate evidence pointing at the same actual source and exact transform
    // does not create a false ambiguity.
    assert(bank.add(make_candidate(game, upstream, pcm.data(), pcm.size())));
    assert(bank.find_automatic(game) != nullptr);

    // A second different "approved" upstream object for the same BRR identity
    // is historical ambiguity. Playback must fall back rather than arbitrarily
    // choose whichever source happened to be inserted first.
    auto conflicting = make_candidate(game, {5, 6}, pcm.data(), pcm.size());
    assert(bank.add(conflicting));
    assert(bank.find_automatic(game) == nullptr);

    bank.clear();
    auto unvalidated = make_candidate(game, upstream, pcm.data(), pcm.size());
    unvalidated.identity_validation_passed = false;
    assert(bank.add(unvalidated));
    assert(bank.find_automatic(game) == nullptr);

    assert(bank.find_automatic({}) == nullptr);

    return 0;
}
