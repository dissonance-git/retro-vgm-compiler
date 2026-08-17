#pragma once

#include "spc_sample_restoration.h"

#include <array>
#include <cstddef>

namespace gameaudio::spc {

// Fixed-capacity runtime registry for already-loaded, evidence-qualified
// upstream sample assets. File discovery, decoding, hashing and provenance live
// at the shell/corpus boundary; the realtime path only sees immutable candidate
// views whose PCM storage outlives the registry.
//
// One game-BRR content identity maps to at most one active automatic candidate.
// Competing or weaker hypotheses remain research records outside this registry
// until adjudication selects one exact preparation lineage.
template <std::size_t MaxEntries>
class spc_sample_restoration_registry {
public:
    struct entry {
        spc_sample_restoration_candidate candidate{};
        bool occupied = false;
    };

    bool insert_automatic(const spc_sample_restoration_candidate& candidate) noexcept {
        if (!may_use_spc_sample_restoration_automatically(candidate))
            return false;

        // Duplicate game identities are rejected even when they point to the
        // same upstream bytes. Ambiguity must be resolved before the realtime
        // owner is populated rather than by insertion order.
        if (find(candidate.game_brr_identity) != nullptr)
            return false;

        for (auto& item : entries_) {
            if (!item.occupied) {
                item.candidate = candidate;
                item.occupied = true;
                ++size_;
                return true;
            }
        }
        return false;
    }

    const spc_sample_restoration_candidate* find(
        const spc_sample_content_identity& game_brr_identity) const noexcept {
        if (!game_brr_identity.present())
            return nullptr;
        for (const auto& item : entries_) {
            if (item.occupied
                && same_spc_sample_content_identity(
                    item.candidate.game_brr_identity,
                    game_brr_identity))
                return &item.candidate;
        }
        return nullptr;
    }

    void clear() noexcept {
        entries_ = {};
        size_ = 0;
    }

    std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return MaxEntries; }

private:
    std::array<entry, MaxEntries> entries_{};
    std::size_t size_ = 0;
};

} // namespace gameaudio::spc
