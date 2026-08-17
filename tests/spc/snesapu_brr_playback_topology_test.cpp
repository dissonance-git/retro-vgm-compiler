#include "components/spc/snesapu_brr_playback_topology.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
using namespace gameaudio::spc;

void write_ram16(
    std::array<std::uint8_t, spc_ram_size>& ram,
    std::uint16_t address,
    std::uint16_t value)
{
    ram[address] = static_cast<std::uint8_t>(value);
    ram[static_cast<std::uint16_t>(address + 1u)]
        = static_cast<std::uint8_t>(value >> 8u);
}

void write_wrapped_bytes(
    std::array<std::uint8_t, spc_ram_size>& ram,
    std::uint16_t first,
    const std::uint8_t* bytes,
    std::size_t size)
{
    for (std::size_t offset = 0; offset < size; ++offset) {
        const auto address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(first)
            + static_cast<std::uint32_t>(offset));
        ram[address] = bytes[offset];
    }
}

std::array<std::uint8_t, 4u * snesapu_topology_brr_bytes_per_block>
make_wrapped_loop_brr() {
    std::array<std::uint8_t, 4u * snesapu_topology_brr_bytes_per_block> brr{};
    for (std::size_t block = 0; block < 4u; ++block) {
        brr[block * snesapu_topology_brr_bytes_per_block]
            = block == 3u ? 0x03u : 0x00u;
        for (std::size_t byte = 1; byte < snesapu_topology_brr_bytes_per_block; ++byte) {
            brr[block * snesapu_topology_brr_bytes_per_block + byte]
                = static_cast<std::uint8_t>(block * 19u + byte);
        }
    }
    return brr;
}
}

int main() {
    using namespace gameaudio::spc;

    // The source starts near the end of SPC RAM. Four sequential BRR blocks
    // therefore cross FFFF -> 0000. The live DIR loop pointer selects block 2.
    std::array<std::uint8_t, spc_ram_size> ram{};
    constexpr std::uint8_t dir_page = 0x80u;
    constexpr std::uint8_t srcn = 0u;
    constexpr std::uint16_t dir_entry = 0x8000u;
    constexpr std::uint16_t first_brr = 0xfff0u;
    constexpr std::uint16_t loop_brr = 0x0002u;
    write_ram16(ram, dir_entry, first_brr);
    write_ram16(ram, static_cast<std::uint16_t>(dir_entry + 2u), loop_brr);

    const auto brr = make_wrapped_loop_brr();
    write_wrapped_bytes(ram, first_brr, brr.data(), brr.size());

    const auto topology = derive_snesapu_brr_playback_topology_from_ram(
        ram.data(), ram.size(), dir_page, srcn);
    assert(topology.valid);
    assert(topology.end_found);
    assert(topology.loop_present);
    assert(topology.loop_target_in_witness);
    assert(topology.directory_entry_address == dir_entry);
    assert(topology.first_brr_block_address == first_brr);
    assert(topology.directory_loop_brr_block_address == loop_brr);
    assert(topology.terminal_brr_header == 0x03u);
    assert(topology.brr_block_count == 4u);
    assert(topology.loop_block_ordinal == 2u);
    assert(topology.game_end_sample() == 64u);
    assert(topology.game_loop_start_sample() == 32u);
    assert(topology.witness_byte_count() == brr.size());

    const auto playback = topology.playback_span();
    assert(playback.valid());
    assert(playback.start_sample == 0.0);
    assert(playback.end_sample == 64.0);
    assert(playback.loop.present);
    assert(playback.loop.start_sample == 32.0);
    assert(playback.loop.end_sample == 64.0);

    std::array<std::uint8_t, 4u * snesapu_topology_brr_bytes_per_block> witness{};
    assert(copy_snesapu_brr_witness_from_ram(
        ram.data(), ram.size(), topology, witness.data(), witness.size()));
    assert(std::memcmp(witness.data(), brr.data(), brr.size()) == 0);
    assert(!copy_snesapu_brr_witness_from_ram(
        ram.data(), ram.size(), topology, witness.data(), witness.size() - 1u));

    // The snapshot wrapper must derive the same geometry from the exact RAM
    // image carried by a normal SPC file.
    std::array<std::uint8_t, spc_min_file_size> spc{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data";
    std::memcpy(spc.data(), signature, sizeof(signature) - 1u);
    std::memcpy(spc.data() + spc_ram_offset, ram.data(), ram.size());
    const auto from_spc = derive_snesapu_brr_playback_topology_from_spc(
        spc.data(), spc.size(), dir_page, srcn);
    assert(from_spc.valid);
    assert(from_spc.first_brr_block_address == first_brr);
    assert(from_spc.loop_block_ordinal == 2u);

    // DIR + SRCN arithmetic itself is 16-bit. SRCN 64 on page FF wraps the
    // directory record base from 10000 to 0000; topology discovery must follow
    // the hardware address rather than rejecting the wrap.
    std::array<std::uint8_t, spc_ram_size> wrapped_directory_ram{};
    constexpr std::uint8_t wrapped_dir_page = 0xffu;
    constexpr std::uint8_t wrapped_srcn = 64u;
    constexpr std::uint16_t wrapped_first = 0x2000u;
    write_ram16(wrapped_directory_ram, 0x0000u, wrapped_first);
    write_ram16(wrapped_directory_ram, 0x0002u, 0x7777u);
    wrapped_directory_ram[wrapped_first] = 0x01u;
    const auto wrapped_directory = derive_snesapu_brr_playback_topology_from_ram(
        wrapped_directory_ram.data(),
        wrapped_directory_ram.size(),
        wrapped_dir_page,
        wrapped_srcn);
    assert(wrapped_directory.valid);
    assert(wrapped_directory.directory_entry_address == 0x0000u);
    assert(wrapped_directory.first_brr_block_address == wrapped_first);
    assert(!wrapped_directory.loop_present);
    assert(wrapped_directory.brr_block_count == 1u);
    assert(wrapped_directory.game_end_sample() == 16u);

    // An END+LOOP sample is not packet-safe unless the live DIR loop pointer is
    // exactly one of the block starts already witnessed before END. A future or
    // byte-misaligned target is hardware state we refuse to reinterpret.
    auto future_loop_ram = ram;
    write_ram16(future_loop_ram, static_cast<std::uint16_t>(dir_entry + 2u), 0x0014u);
    const auto future_loop = derive_snesapu_brr_playback_topology_from_ram(
        future_loop_ram.data(), future_loop_ram.size(), dir_page, srcn);
    assert(!future_loop.valid);
    assert(future_loop.end_found);
    assert(future_loop.loop_present);
    assert(!future_loop.loop_target_in_witness);
    assert(future_loop.brr_block_count == 4u);

    auto misaligned_loop_ram = ram;
    write_ram16(misaligned_loop_ram, static_cast<std::uint16_t>(dir_entry + 2u), 0x0003u);
    const auto misaligned_loop = derive_snesapu_brr_playback_topology_from_ram(
        misaligned_loop_ram.data(), misaligned_loop_ram.size(), dir_page, srcn);
    assert(!misaligned_loop.valid);
    assert(misaligned_loop.end_found);
    assert(misaligned_loop.loop_present);

    // With END but no LOOP flag, the directory's loop word is irrelevant to
    // this playback trajectory and must not manufacture a loop.
    auto one_shot_ram = ram;
    one_shot_ram[static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(first_brr)
        + 3u * snesapu_topology_brr_bytes_per_block)] = 0x01u;
    write_ram16(one_shot_ram, static_cast<std::uint16_t>(dir_entry + 2u), 0x4567u);
    const auto one_shot = derive_snesapu_brr_playback_topology_from_ram(
        one_shot_ram.data(), one_shot_ram.size(), dir_page, srcn);
    assert(one_shot.valid);
    assert(one_shot.end_found);
    assert(!one_shot.loop_present);
    assert(one_shot.loop_block_ordinal == snesapu_topology_no_loop);
    assert(one_shot.game_loop_start_sample() == snesapu_topology_no_loop);

    // No END within the bounded one-RAM-image witness budget is deliberately
    // unsupported. We do not invent a terminal boundary or create an unbounded
    // transport packet.
    std::array<std::uint8_t, spc_ram_size> endless_ram{};
    const auto endless = derive_snesapu_brr_playback_topology_from_ram(
        endless_ram.data(), endless_ram.size(), 0u, 0u);
    assert(!endless.valid);
    assert(!endless.end_found);
    assert(endless.brr_block_count == 0u);

    assert(!derive_snesapu_brr_playback_topology_from_ram(
        ram.data(), ram.size() - 1u, dir_page, srcn).valid);
    assert(!derive_snesapu_brr_playback_topology_from_spc(
        spc.data(), spc_ram_offset + spc_ram_size - 1u, dir_page, srcn).valid);

    return 0;
}
