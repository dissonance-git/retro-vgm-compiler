#pragma once

#include "snesapu_studio_source_packet.h"

namespace gameaudio::spc {

// Setup-time owner used in the 32-bit spcplayer child. In addition to parsing
// the bounded packet, it byte-compares every serialized BRR source against the
// actual 64 KiB RAM image in the SPC being loaded. Thus a stale/renamed sidecar
// cannot become source truth merely because SRCN and addresses happen to line up.
template <std::size_t MaxEntries = snes_studio_source_max_entries>
class snes_studio_source_packet_runtime {
    static_assert(MaxEntries > 0 && MaxEntries <= snes_studio_source_max_entries,
        "studio source runtime capacity must be 1..256");

public:
    bool load(
        const std::uint8_t* data,
        std::size_t size,
        const std::uint8_t* spc_data,
        std::size_t spc_size)
    {
        clear();
        snes_studio_source_packet_view view;
        if (!view.reset(data, size) || view.entry_count() > MaxEntries
            || !has_spc_signature(spc_data, spc_size)
            || spc_size < spc_min_file_size)
            return false;

        for (std::size_t index = 0; index < view.entry_count(); ++index) {
            const auto item = view.entry(index);
            const std::uint8_t* raw_brr = view.brr_bytes(index);
            const std::uint64_t brr_size64 = static_cast<std::uint64_t>(item.brr_block_count)
                * snesapu_brr_bytes_per_block;
            if (raw_brr == nullptr
                || brr_size64 > std::numeric_limits<std::size_t>::max()
                || !snes_studio_brr_headers_match_playback(
                    raw_brr,
                    item.brr_block_count,
                    item.loop_present)
                || !snes_studio_brr_matches_spc_snapshot(
                    spc_data,
                    spc_size,
                    item.first_brr_block_address,
                    raw_brr,
                    static_cast<std::size_t>(brr_size64)))
                return fail();

            auto& pcm = pcm_by_entry_[index];
            pcm.resize(item.pcm_frame_count);
            const std::uint8_t* raw_pcm = view.pcm_bytes(index);
            if (raw_pcm == nullptr)
                return fail();
            for (std::size_t frame = 0; frame < pcm.size(); ++frame) {
                pcm[frame] = snes_studio_read_f32(raw_pcm + frame * sizeof(float));
                if (!std::isfinite(pcm[frame]))
                    return fail();
            }

            auto& candidate = candidates_[index];
            candidate = {};
            candidate.game_brr_identity = item.game_brr_identity;
            candidate.upstream_identity = item.upstream_identity;
            candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
            // The packet boundary never upgrades provenance. Runtime reconstructs
            // the weakest evidence grade that is still source-supported automatic.
            candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
            candidate.basis = spc_sample_restoration_basis::exact_upstream_pcm;
            candidate.upstream = {
                pcm.data(),
                pcm.size(),
                item.sample_rate_hz,
                item.game_pcm_units_per_source_unit,
            };
            candidate.coordinate_map.game_origin = item.game_origin;
            candidate.coordinate_map.upstream_origin = item.upstream_origin;
            candidate.coordinate_map.upstream_frames_per_game_sample =
                item.upstream_frames_per_game_sample;
            candidate.coordinate_map.loop_present = item.loop_present;
            candidate.coordinate_map.game_loop_start = item.loop_present
                ? static_cast<double>(item.game_loop_start_sample())
                : 0.0;
            candidate.coordinate_map.upstream_loop_start = item.loop_present
                ? item.upstream_loop_start
                : 0.0;
            candidate.coordinate_map.preparation_chain_exact = true;
            candidate.identity_validation_passed = true;

            spc_game_sample_playback_span playback;
            playback.start_sample = 0.0;
            playback.end_sample = static_cast<double>(item.game_end_sample());
            if (item.loop_present) {
                playback.loop = {
                    true,
                    static_cast<double>(item.game_loop_start_sample()),
                    static_cast<double>(item.game_end_sample()),
                };
            }

            if (!provider_.add({
                    item.source_number,
                    item.first_brr_block_address,
                    item.loop_brr_block_address(),
                    &candidate,
                    playback,
                }))
                return fail();
        }

        loaded_ = provider_.source_count() == view.entry_count();
        return loaded_;
    }

    void clear() noexcept {
        provider_.clear();
        for (auto& pcm : pcm_by_entry_)
            pcm.clear();
        candidates_ = {};
        loaded_ = false;
    }

    [[nodiscard]] bool loaded() const noexcept { return loaded_; }
    [[nodiscard]] std::size_t source_count() const noexcept {
        return provider_.source_count();
    }

    snesapu_studio_source_provider<MaxEntries>& provider() noexcept {
        return provider_;
    }

    const snesapu_studio_source_provider<MaxEntries>& provider() const noexcept {
        return provider_;
    }

private:
    bool fail() noexcept {
        clear();
        return false;
    }

    std::array<std::vector<float>, MaxEntries> pcm_by_entry_{};
    std::array<spc_sample_restoration_candidate, MaxEntries> candidates_{};
    snesapu_studio_source_provider<MaxEntries> provider_{};
    bool loaded_ = false;
};

} // namespace gameaudio::spc
