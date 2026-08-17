#pragma once

#include "snesapu_brr_playback_topology.h"
#include "snesapu_studio_source_packet.h"

namespace gameaudio::spc {

class snes_studio_source_packet_builder {
public:
    struct source {
        std::uint8_t source_number = 0;
        std::uint16_t first_brr_block_address = 0;
        const std::uint8_t* game_brr_bytes = nullptr;
        std::size_t game_brr_byte_count = 0;
        const spc_sample_restoration_candidate* restoration = nullptr;
        spc_game_sample_playback_span playback{};
    };

    // Preferred authoring input when an SPC snapshot is available. The caller
    // supplies only the runtime source selector, the live DIR page represented
    // by the snapshot, and the already-admitted upstream restoration. BRR start,
    // END/LOOP topology, loop target and the exact compressed witness are then
    // derived from the snapshot itself rather than copied from hand-authored
    // sidecar metadata.
    struct snapshot_source {
        std::uint8_t source_number = 0;
        std::uint8_t directory_page = 0;
        const std::uint8_t* spc_data = nullptr;
        std::size_t spc_size = 0;
        const spc_sample_restoration_candidate* restoration = nullptr;
    };

    bool build_from_spc_snapshot(const snapshot_source* sources, std::size_t count) {
        bytes_.clear();
        if (sources == nullptr || count == 0
            || count > snes_studio_source_max_entries)
            return false;

        std::vector<std::vector<std::uint8_t>> witnesses(count);
        std::vector<source> resolved(count);

        for (std::size_t index = 0; index < count; ++index) {
            const snapshot_source& item = sources[index];
            if (item.restoration == nullptr)
                return false;

            const auto topology = derive_snesapu_brr_playback_topology_from_spc(
                item.spc_data,
                item.spc_size,
                item.directory_page,
                item.source_number);
            if (!topology.valid)
                return false;

            auto& witness = witnesses[index];
            witness.resize(topology.witness_byte_count());
            if (!copy_snesapu_brr_witness_from_ram(
                    item.spc_data + spc_ram_offset,
                    spc_ram_size,
                    topology,
                    witness.data(),
                    witness.size()))
                return false;

            resolved[index] = {
                item.source_number,
                topology.first_brr_block_address,
                witness.data(),
                witness.size(),
                item.restoration,
                topology.playback_span(),
            };
        }

        return build(resolved.data(), resolved.size());
    }

    bool build(const source* sources, std::size_t count) {
        bytes_.clear();
        if (sources == nullptr || count == 0
            || count > snes_studio_source_max_entries
            || count > std::numeric_limits<std::uint16_t>::max())
            return false;

        std::array<std::uint32_t, snes_studio_source_max_entries> block_count{};
        std::array<std::uint32_t, snes_studio_source_max_entries> loop_block{};
        std::array<std::uint32_t, snes_studio_source_max_entries> brr_offset{};
        std::array<std::uint32_t, snes_studio_source_max_entries> pcm_offset{};

        std::uint64_t total = snes_studio_source_packet_header_size
            + static_cast<std::uint64_t>(count) * snes_studio_source_packet_entry_size;

        for (std::size_t index = 0; index < count; ++index) {
            const source& item = sources[index];
            if (item.restoration == nullptr
                || item.game_brr_bytes == nullptr
                || item.restoration->relation != spc_sample_lineage_relation::exact_pre_brr_source
                || !may_use_spc_sample_restoration_automatically(*item.restoration)
                || !item.playback.valid()
                || std::abs(item.playback.start_sample) > 1.0e-12
                || !detail::resolve_spc_upstream_playback_boundaries(
                        *item.restoration, item.playback).valid)
                return false;

            std::int64_t end_integer = 0;
            if (!detail::spc_near_integer(item.playback.end_sample, end_integer)
                || end_integer <= 0
                || (static_cast<std::uint64_t>(end_integer)
                    % snesapu_brr_samples_per_block) != 0u)
                return false;
            const std::uint64_t blocks64 = static_cast<std::uint64_t>(end_integer)
                / snesapu_brr_samples_per_block;
            if (blocks64 == 0
                || blocks64 > spc_ram_size / snesapu_brr_bytes_per_block
                || blocks64 > std::numeric_limits<std::uint32_t>::max())
                return false;
            block_count[index] = static_cast<std::uint32_t>(blocks64);

            const std::uint64_t expected_brr_bytes = blocks64 * snesapu_brr_bytes_per_block;
            if (item.game_brr_byte_count != expected_brr_bytes
                || expected_brr_bytes > spc_ram_size
                || !snes_studio_brr_headers_match_playback(
                    item.game_brr_bytes,
                    static_cast<std::size_t>(blocks64),
                    item.playback.loop.present))
                return false;

            loop_block[index] = snes_studio_source_no_loop;
            if (item.playback.loop.present) {
                std::int64_t loop_integer = 0;
                if (!detail::spc_near_integer(
                        item.playback.loop.start_sample, loop_integer)
                    || loop_integer < 0
                    || loop_integer >= end_integer
                    || (static_cast<std::uint64_t>(loop_integer)
                        % snesapu_brr_samples_per_block) != 0u)
                    return false;
                loop_block[index] = static_cast<std::uint32_t>(
                    static_cast<std::uint64_t>(loop_integer)
                    / snesapu_brr_samples_per_block);
            }

            for (std::size_t prior = 0; prior < index; ++prior) {
                if (sources[prior].source_number == item.source_number
                    && sources[prior].first_brr_block_address
                        == item.first_brr_block_address)
                    return false;
            }

            const auto& upstream = item.restoration->upstream;
            if (upstream.frame_count > std::numeric_limits<std::uint32_t>::max())
                return false;
            const std::uint64_t pcm_bytes = static_cast<std::uint64_t>(upstream.frame_count)
                * sizeof(float);
            if (pcm_bytes > std::numeric_limits<std::uint32_t>::max())
                return false;

            if (total > std::numeric_limits<std::uint32_t>::max()
                || expected_brr_bytes > std::numeric_limits<std::uint32_t>::max() - total)
                return false;
            brr_offset[index] = static_cast<std::uint32_t>(total);
            total += expected_brr_bytes;
            total = (total + 3u) & ~std::uint64_t{3u};
            if (total > std::numeric_limits<std::uint32_t>::max()
                || pcm_bytes > std::numeric_limits<std::uint32_t>::max() - total)
                return false;
            pcm_offset[index] = static_cast<std::uint32_t>(total);
            total += pcm_bytes;
        }
        if (total > std::numeric_limits<std::uint32_t>::max()
            || total > std::numeric_limits<std::size_t>::max())
            return false;

        bytes_.assign(static_cast<std::size_t>(total), 0u);
        snes_studio_write_le32(bytes_.data(), snes_studio_source_packet_magic);
        snes_studio_write_le16(bytes_.data() + 4, snes_studio_source_packet_version);
        snes_studio_write_le16(
            bytes_.data() + 6,
            static_cast<std::uint16_t>(snes_studio_source_packet_header_size));
        snes_studio_write_le16(bytes_.data() + 8, static_cast<std::uint16_t>(count));
        snes_studio_write_le16(bytes_.data() + 10, 0u);
        snes_studio_write_le32(bytes_.data() + 12, static_cast<std::uint32_t>(total));

        for (std::size_t index = 0; index < count; ++index) {
            const source& item = sources[index];
            const auto& candidate = *item.restoration;
            std::uint8_t* raw = bytes_.data() + snes_studio_source_packet_header_size
                + index * snes_studio_source_packet_entry_size;
            raw[0] = item.source_number;
            raw[1] = item.playback.loop.present ? snes_studio_source_flag_loop : 0u;
            snes_studio_write_le16(raw + 2, item.first_brr_block_address);
            snes_studio_write_le32(raw + 4, block_count[index]);
            snes_studio_write_le32(raw + 8, loop_block[index]);
            snes_studio_write_le32(
                raw + 12,
                static_cast<std::uint32_t>(candidate.upstream.frame_count));
            snes_studio_write_le32(raw + 16, brr_offset[index]);
            snes_studio_write_le32(raw + 20, pcm_offset[index]);
            snes_studio_write_le64(raw + 24, candidate.game_brr_identity.high);
            snes_studio_write_le64(raw + 32, candidate.game_brr_identity.low);
            snes_studio_write_le64(raw + 40, candidate.upstream_identity.high);
            snes_studio_write_le64(raw + 48, candidate.upstream_identity.low);
            snes_studio_write_f64(raw + 56, candidate.coordinate_map.game_origin);
            snes_studio_write_f64(raw + 64, candidate.coordinate_map.upstream_origin);
            snes_studio_write_f64(
                raw + 72, candidate.coordinate_map.upstream_frames_per_game_sample);
            snes_studio_write_f64(
                raw + 80,
                item.playback.loop.present
                    ? candidate.coordinate_map.upstream_loop_start
                    : 0.0);
            snes_studio_write_f64(raw + 88, candidate.upstream.sample_rate_hz);
            snes_studio_write_f64(
                raw + 96, candidate.upstream.game_pcm_units_per_source_unit);
            snes_studio_write_le64(raw + 104, 0u);
            snes_studio_write_le64(raw + 112, 0u);

            std::memcpy(
                bytes_.data() + brr_offset[index],
                item.game_brr_bytes,
                item.game_brr_byte_count);

            std::size_t payload = pcm_offset[index];
            for (std::size_t frame = 0; frame < candidate.upstream.frame_count; ++frame) {
                const float sample = candidate.upstream.mono_pcm[frame];
                if (!std::isfinite(sample)) {
                    bytes_.clear();
                    return false;
                }
                snes_studio_write_f32(bytes_.data() + payload, sample);
                payload += sizeof(float);
            }
        }
        return true;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept {
        return bytes_;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

} // namespace gameaudio::spc
