#pragma once

#include "spc_snapshot.h"
#include "spc_upstream_playback_reconstruction.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

// Hardware facts used by the topology reader. These intentionally have local
// names instead of duplicating the studio-provider constants: this header is a
// lower-level snapshot utility and can later become the common home for them.
constexpr std::uint32_t snesapu_topology_brr_samples_per_block = 16u;
constexpr std::uint32_t snesapu_topology_brr_bytes_per_block = 9u;
constexpr std::uint32_t snesapu_topology_no_loop =
    std::numeric_limits<std::uint32_t>::max();

// The studio packet deliberately refuses a BRR witness larger than one SPC RAM
// image. A sequential 9-byte walk can in principle revisit RAM after many more
// blocks because 9 and 65536 are coprime, but such a witness would duplicate
// snapshot bytes and exceed the bounded transport contract. Stop at the same
// exactness boundary here rather than silently truncating it.
constexpr std::uint32_t snesapu_topology_max_brr_blocks =
    static_cast<std::uint32_t>(spc_ram_size / snesapu_topology_brr_bytes_per_block);

struct snesapu_brr_playback_topology {
    bool valid = false;
    bool end_found = false;
    bool loop_present = false;
    bool loop_target_in_witness = false;

    std::uint16_t directory_entry_address = 0;
    std::uint16_t first_brr_block_address = 0;
    std::uint16_t directory_loop_brr_block_address = 0;
    std::uint8_t terminal_brr_header = 0;

    std::uint32_t brr_block_count = 0;
    std::uint32_t loop_block_ordinal = snesapu_topology_no_loop;

    [[nodiscard]] std::uint32_t game_end_sample() const noexcept {
        return brr_block_count * snesapu_topology_brr_samples_per_block;
    }

    [[nodiscard]] std::uint32_t game_loop_start_sample() const noexcept {
        return loop_present && loop_target_in_witness
            ? loop_block_ordinal * snesapu_topology_brr_samples_per_block
            : snesapu_topology_no_loop;
    }

    [[nodiscard]] std::size_t witness_byte_count() const noexcept {
        return static_cast<std::size_t>(brr_block_count)
            * snesapu_topology_brr_bytes_per_block;
    }

    [[nodiscard]] spc_game_sample_playback_span playback_span() const noexcept {
        if (!valid)
            return {};
        spc_game_sample_playback_span span;
        span.start_sample = 0.0;
        span.end_sample = static_cast<double>(game_end_sample());
        if (loop_present) {
            span.loop.present = true;
            span.loop.start_sample = static_cast<double>(game_loop_start_sample());
            span.loop.end_sample = span.end_sample;
        }
        return span;
    }
};

inline std::uint8_t snesapu_topology_ram_read8(
    const std::uint8_t* ram,
    std::uint16_t address) noexcept
{
    return ram[address];
}

inline std::uint16_t snesapu_topology_ram_read16(
    const std::uint8_t* ram,
    std::uint16_t address) noexcept
{
    const std::uint8_t low = snesapu_topology_ram_read8(ram, address);
    const std::uint8_t high = snesapu_topology_ram_read8(
        ram, static_cast<std::uint16_t>(address + 1u));
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(low)
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(high) << 8u));
}

inline snesapu_brr_playback_topology derive_snesapu_brr_playback_topology_from_ram(
    const std::uint8_t* ram,
    std::size_t ram_size,
    std::uint8_t directory_page,
    std::uint8_t source_number) noexcept
{
    snesapu_brr_playback_topology result;
    if (ram == nullptr || ram_size < spc_ram_size)
        return result;

    // DIR is a page base and each SRCN directory record is four bytes:
    // start address (LE16), loop address (LE16). All SPC RAM addressing wraps
    // at 16 bits, including a directory record crossing FFFF -> 0000.
    result.directory_entry_address = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(directory_page) << 8u)
        + static_cast<std::uint32_t>(source_number) * 4u);
    result.first_brr_block_address = snesapu_topology_ram_read16(
        ram, result.directory_entry_address);
    result.directory_loop_brr_block_address = snesapu_topology_ram_read16(
        ram, static_cast<std::uint16_t>(result.directory_entry_address + 2u));

    for (std::uint32_t block = 0; block < snesapu_topology_max_brr_blocks; ++block) {
        const std::uint16_t address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(result.first_brr_block_address)
            + block * snesapu_topology_brr_bytes_per_block);
        const std::uint8_t header = snesapu_topology_ram_read8(ram, address);
        if ((header & 0x01u) == 0u)
            continue;

        result.end_found = true;
        result.terminal_brr_header = header;
        result.brr_block_count = block + 1u;
        result.loop_present = (header & 0x02u) != 0u;

        if (!result.loop_present) {
            result.loop_target_in_witness = false;
            result.loop_block_ordinal = snesapu_topology_no_loop;
            result.valid = true;
            return result;
        }

        // Map the live directory loop address back onto the exact sequence of
        // BRR block starts already witnessed from first through END. The walk
        // is shorter than 65536 bytes, so the 16-bit delta is unambiguous here.
        const std::uint32_t delta = static_cast<std::uint16_t>(
            result.directory_loop_brr_block_address
            - result.first_brr_block_address);
        if ((delta % snesapu_topology_brr_bytes_per_block) != 0u)
            return result;

        const std::uint32_t loop_block = delta / snesapu_topology_brr_bytes_per_block;
        if (loop_block >= result.brr_block_count)
            return result;

        result.loop_block_ordinal = loop_block;
        result.loop_target_in_witness = true;
        result.valid = true;
        return result;
    }

    return result;
}

inline snesapu_brr_playback_topology derive_snesapu_brr_playback_topology_from_spc(
    const std::uint8_t* spc_data,
    std::size_t spc_size,
    std::uint8_t directory_page,
    std::uint8_t source_number) noexcept
{
    if (!has_spc_signature(spc_data, spc_size)
        || spc_size < spc_ram_offset + spc_ram_size)
        return {};
    return derive_snesapu_brr_playback_topology_from_ram(
        spc_data + spc_ram_offset,
        spc_ram_size,
        directory_page,
        source_number);
}

inline bool copy_snesapu_brr_witness_from_ram(
    const std::uint8_t* ram,
    std::size_t ram_size,
    const snesapu_brr_playback_topology& topology,
    std::uint8_t* destination,
    std::size_t destination_size) noexcept
{
    if (ram == nullptr || ram_size < spc_ram_size || !topology.valid
        || destination == nullptr
        || destination_size != topology.witness_byte_count())
        return false;

    for (std::size_t offset = 0; offset < destination_size; ++offset) {
        const std::uint16_t address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(topology.first_brr_block_address)
            + static_cast<std::uint32_t>(offset));
        destination[offset] = snesapu_topology_ram_read8(ram, address);
    }
    return true;
}

} // namespace gameaudio::spc
