#include "components/spc/snesapu_studio_source_packet_builder.h"
#include "components/spc/snesapu_studio_source_packet_runtime.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace {
bool near(double a, double b, double tolerance = 2.0e-5) {
    return std::abs(a - b) <= tolerance;
}

std::array<std::uint8_t, gameaudio::spc::spc_full_file_size> make_spc() {
    using namespace gameaudio::spc;
    std::array<std::uint8_t, spc_full_file_size> spc{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data";
    std::memcpy(spc.data(), signature, sizeof(signature) - 1u);
    return spc;
}

void install_wrapped_brr(
    std::array<std::uint8_t, gameaudio::spc::spc_full_file_size>& spc,
    std::uint16_t first,
    const std::uint8_t* brr,
    std::size_t size)
{
    using namespace gameaudio::spc;
    for (std::size_t offset = 0; offset < size; ++offset) {
        const std::uint16_t address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(first) + static_cast<std::uint32_t>(offset));
        spc[spc_ram_offset + address] = brr[offset];
    }
}

void write_spc_ram16(
    std::array<std::uint8_t, gameaudio::spc::spc_full_file_size>& spc,
    std::uint16_t address,
    std::uint16_t value)
{
    using namespace gameaudio::spc;
    spc[spc_ram_offset + address] = static_cast<std::uint8_t>(value);
    spc[spc_ram_offset + static_cast<std::uint16_t>(address + 1u)]
        = static_cast<std::uint8_t>(value >> 8u);
}
}

int main() {
    using namespace gameaudio::spc;

    std::array<float, 96> upstream{};
    for (std::size_t index = 0; index < upstream.size(); ++index) {
        upstream[index] = static_cast<float>(
            0.29 * std::sin(static_cast<double>(index) * 0.33)
            + 0.17 * std::cos(static_cast<double>(index) * 0.57));
    }

    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {0x1111u, 0x2222u};
    candidate.upstream_identity = {0x3333u, 0x4444u};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.basis = spc_sample_restoration_basis::exact_upstream_pcm;
    candidate.upstream = {upstream.data(), upstream.size(), 48000.0, 1.0};
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 0.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.0;
    candidate.coordinate_map.loop_present = true;
    candidate.coordinate_map.game_loop_start = 32.0;
    candidate.coordinate_map.upstream_loop_start = 32.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;

    const spc_game_sample_playback_span playback{
        0.0, 64.0, {true, 32.0, 64.0}};

    // Four BRR blocks start at FFF0, so the sequence wraps through address 0000.
    // Block two (game sample 32) begins at 0002 and is the live loop target.
    std::array<std::uint8_t, 4u * snesapu_brr_bytes_per_block> brr{};
    for (std::size_t block = 0; block < 4; ++block) {
        brr[block * snesapu_brr_bytes_per_block] = block == 3 ? 0x03u : 0x00u;
        for (std::size_t byte = 1; byte < snesapu_brr_bytes_per_block; ++byte) {
            brr[block * snesapu_brr_bytes_per_block + byte]
                = static_cast<std::uint8_t>(block * 17u + byte);
        }
    }

    auto spc = make_spc();
    constexpr std::uint8_t directory_page = 0x4cu;
    constexpr std::uint8_t source_number = 7u;
    constexpr std::uint16_t directory_entry =
        static_cast<std::uint16_t>((directory_page << 8u) + source_number * 4u);
    constexpr std::uint16_t first_brr = 0xfff0u;
    constexpr std::uint16_t loop_brr = 0x0002u;
    install_wrapped_brr(spc, first_brr, brr.data(), brr.size());
    write_spc_ram16(spc, directory_entry, first_brr);
    write_spc_ram16(spc, static_cast<std::uint16_t>(directory_entry + 2u), loop_brr);

    snes_studio_source_packet_builder builder;
    const snes_studio_source_packet_builder::source source{
        source_number,
        first_brr,
        brr.data(),
        brr.size(),
        &candidate,
        playback,
    };
    assert(builder.build(&source, 1));
    assert(!builder.bytes().empty());

    // Preferred authoring path: derive first/loop addresses, BRR witness and
    // game playback span from the actual SPC snapshot. With identical evidence
    // it must serialize byte-for-byte identically to the explicit legacy input.
    snes_studio_source_packet_builder snapshot_builder;
    const snes_studio_source_packet_builder::snapshot_source snapshot_source{
        source_number,
        directory_page,
        spc.data(),
        spc.size(),
        &candidate,
    };
    assert(snapshot_builder.build_from_spc_snapshot(&snapshot_source, 1));
    assert(snapshot_builder.bytes() == builder.bytes());

    // A live DIR loop pointer that is not one of the witnessed BRR block starts
    // cannot be papered over by caller metadata. Snapshot-derived authoring
    // fails closed before a sidecar exists.
    auto wrong_directory_spc = spc;
    write_spc_ram16(
        wrong_directory_spc,
        static_cast<std::uint16_t>(directory_entry + 2u),
        0x0003u);
    const snes_studio_source_packet_builder::snapshot_source wrong_snapshot_source{
        source_number,
        directory_page,
        wrong_directory_spc.data(),
        wrong_directory_spc.size(),
        &candidate,
    };
    assert(!snapshot_builder.build_from_spc_snapshot(&wrong_snapshot_source, 1));

    snes_studio_source_packet_view view;
    assert(view.reset(builder.bytes().data(), builder.bytes().size()));
    assert(view.entry_count() == 1);
    const auto entry = view.entry(0);
    assert(entry.source_number == source_number);
    assert(entry.first_brr_block_address == first_brr);
    assert(entry.loop_present);
    assert(entry.brr_block_count == 4u);
    assert(entry.loop_block_ordinal == 2u);
    assert(entry.game_end_sample() == 64u);
    assert(entry.game_loop_start_sample() == 32u);
    assert(entry.loop_brr_block_address() == loop_brr);
    assert(entry.pcm_frame_count == upstream.size());
    assert(std::memcmp(view.brr_bytes(0), brr.data(), brr.size()) == 0);

    snes_studio_source_packet_runtime<4> runtime;
    assert(runtime.load(
        builder.bytes().data(),
        builder.bytes().size(),
        spc.data(),
        spc.size()));
    assert(runtime.loaded());
    assert(runtime.source_count() == 1);

    auto& provider = runtime.provider();
    assert(provider.begin_voice(
        0, source_number, first_brr, loop_brr, directory_page, 0));
    snesapu_source_trajectory_tracker reference;
    reference.key_on(snesapu_source_interpolation::none);
    const auto projection = reference.project(playback.loop);
    const auto expected = reconstruct_spc_upstream_playback_sample(
        candidate, playback, projection);
    assert(expected.valid);
    float rendered = 0.0f;
    assert(provider.render_voice(
        0,
        0x00010000u,
        source_number,
        loop_brr,
        directory_page,
        0,
        &rendered));
    assert(near(rendered, expected.sample));

    // Runtime identity stays live after setup too. A Script700/SRCN remap or a
    // changed loop locator on the same DIR page must not keep using the packet's
    // old upstream trajectory merely because its original BRR witness was valid.
    assert(provider.begin_voice(
        1, source_number, first_brr, loop_brr, directory_page, 0));
    assert(!provider.render_voice(
        1,
        0x00010000u,
        static_cast<std::uint32_t>(source_number + 1u),
        loop_brr,
        directory_page,
        0,
        &rendered));
    assert(!provider.voice_active(1));

    assert(provider.begin_voice(
        1, source_number, first_brr, loop_brr, directory_page, 0));
    assert(!provider.render_voice(
        1,
        0x00010000u,
        source_number,
        static_cast<std::uint16_t>(loop_brr + snesapu_brr_bytes_per_block),
        directory_page,
        0,
        &rendered));
    assert(!provider.voice_active(1));

    // The transport is content-bound, not path/address-bound. One changed BRR
    // payload byte in the actual SPC rejects the packet even though SRCN and
    // both runtime addresses are still identical.
    auto wrong_spc = spc;
    wrong_spc[spc_ram_offset + 0xfff1u] ^= 0x01u;
    snes_studio_source_packet_runtime<4> wrong_runtime;
    assert(!wrong_runtime.load(
        builder.bytes().data(),
        builder.bytes().size(),
        wrong_spc.data(),
        wrong_spc.size()));

    // Likewise, corrupting the serialized BRR witness without changing its
    // structural headers keeps the packet syntactically valid but fails the
    // exact RAM comparison at runtime.
    auto wrong_packet = builder.bytes();
    snes_studio_source_packet_view wrong_view;
    assert(wrong_view.reset(wrong_packet.data(), wrong_packet.size()));
    const std::size_t brr_offset = wrong_view.entry(0).brr_offset_bytes;
    wrong_packet[brr_offset + 1u] ^= 0x40u;
    assert(wrong_view.reset(wrong_packet.data(), wrong_packet.size()));
    assert(!wrong_runtime.load(
        wrong_packet.data(),
        wrong_packet.size(),
        spc.data(),
        spc.size()));

    // Malformed transport metadata fails before any source object is rebuilt.
    auto reserved_packet = builder.bytes();
    snes_studio_write_le64(
        reserved_packet.data() + snes_studio_source_packet_header_size + 104u,
        1u);
    assert(!wrong_view.reset(reserved_packet.data(), reserved_packet.size()));

    auto bad_magic = builder.bytes();
    bad_magic[0] ^= 0xffu;
    assert(!wrong_view.reset(bad_magic.data(), bad_magic.size()));

    // Non-finite upstream PCM cannot enter either side of the packet boundary.
    auto nan_packet = builder.bytes();
    assert(wrong_view.reset(nan_packet.data(), nan_packet.size()));
    const std::size_t pcm_offset = wrong_view.entry(0).pcm_offset_bytes;
    snes_studio_write_le32(nan_packet.data() + pcm_offset, 0x7fc00000u);
    assert(wrong_view.reset(nan_packet.data(), nan_packet.size()));
    assert(!wrong_runtime.load(
        nan_packet.data(),
        nan_packet.size(),
        spc.data(),
        spc.size()));

    // One runtime locator may not carry two independently approved hypotheses.
    const std::array<snes_studio_source_packet_builder::source, 2> duplicates{
        source, source};
    assert(!builder.build(duplicates.data(), duplicates.size()));

    // The per-sample studio packet is intentionally reserved for actual
    // upstream/pre-BRR originals. A game-grid-only source belongs to the lower
    // pre-BRR packet/rung instead.
    auto game_grid_only = candidate;
    game_grid_only.relation = spc_sample_lineage_relation::exact_source_after_game_preparation;
    const snes_studio_source_packet_builder::source lower_rung{
        8u, 0x2000u, brr.data(), brr.size(), &game_grid_only, playback};
    assert(!builder.build(&lower_rung, 1));

    // Real BRR loop and END boundaries are block-aligned. A fractional playback
    // loop cannot be serialized as if it were exact hardware topology.
    auto fractional = candidate;
    fractional.coordinate_map.upstream_frames_per_game_sample = 1.5;
    fractional.coordinate_map.upstream_loop_start = 48.0;
    const snes_studio_source_packet_builder::source unsupported{
        9u,
        0x3000u,
        brr.data(),
        brr.size(),
        &fractional,
        {0.0, 63.5, {true, 32.0, 63.5}},
    };
    assert(!builder.build(&unsupported, 1));

    return 0;
}
