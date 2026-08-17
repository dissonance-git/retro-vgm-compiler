#pragma once

#include "spc_sample_restoration.h"

#include <array>
#include <cstddef>

namespace gameaudio::spc {

// Runtime lookup table for evidence-approved pre-BRR sample sources. The bank
// does not discover provenance by itself; offline/corpus tooling populates it
// after robust candidate search and lineage verification. Playback only receives
// a source when the game BRR identity resolves to one unambiguous automatic
// candidate.
template <std::size_t MaxCandidates = 1024>
class spc_original_sample_bank {
    static_assert(MaxCandidates > 0, "MaxCandidates must be non-zero");

public:
    bool add(const spc_sample_restoration_candidate& candidate) noexcept {
        if (count_ >= entries_.size())
            return false;
        entries_[count_++] = candidate;
        return true;
    }

    void clear() noexcept { count_ = 0; }
    std::size_t count() const noexcept { return count_; }

    const spc_sample_restoration_candidate* find_automatic(
        const spc_sample_content_identity& game_brr_identity) const noexcept
    {
        if (!game_brr_identity.present())
            return nullptr;

        const spc_sample_restoration_candidate* selected = nullptr;
        for (std::size_t index = 0; index < count_; ++index) {
            const auto& candidate = entries_[index];
            if (!same_spc_sample_content_identity(
                    candidate.game_brr_identity, game_brr_identity))
                continue;
            if (!may_use_spc_sample_restoration_automatically(candidate))
                continue;

            if (selected == nullptr) {
                selected = &candidate;
                continue;
            }

            // Two independently approved entries are harmless only when they
            // resolve to the same upstream content identity and preparation
            // coordinate map. Otherwise playback cannot know which historical
            // source object is the intended one, so ambiguity falls back to BRR.
            if (!same_spc_sample_content_identity(
                    selected->upstream_identity,
                    candidate.upstream_identity)
                || selected->coordinate_map.game_origin != candidate.coordinate_map.game_origin
                || selected->coordinate_map.upstream_origin != candidate.coordinate_map.upstream_origin
                || selected->coordinate_map.upstream_frames_per_game_sample
                    != candidate.coordinate_map.upstream_frames_per_game_sample
                || selected->coordinate_map.loop_present != candidate.coordinate_map.loop_present
                || selected->coordinate_map.game_loop_start != candidate.coordinate_map.game_loop_start
                || selected->coordinate_map.upstream_loop_start
                    != candidate.coordinate_map.upstream_loop_start) {
                return nullptr;
            }
        }
        return selected;
    }

private:
    std::array<spc_sample_restoration_candidate, MaxCandidates> entries_{};
    std::size_t count_ = 0;
};

} // namespace gameaudio::spc
