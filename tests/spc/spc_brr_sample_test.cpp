#include "../../components/spc/spc_brr_sample.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static const attribute* find_attribute(const node* value, const char* key) {
    if (value == nullptr)
        return nullptr;
    for (const auto& item : value->attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

static std::array<std::uint8_t, spc_full_file_size> make_fixture() {
    std::array<std::uint8_t, spc_full_file_size> bytes{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data v0.30\x1A\x1A";
    static_assert(sizeof(signature) - 1 == spc_signature_size, "unexpected SPC signature size");
    std::memcpy(bytes.data(), signature, spc_signature_size);
    bytes[0x24] = 30;

    // All eight saved voice slots reference SRCN 3 or 4. Both directory entries
    // resolve to the same exact RAM start address, so this snapshot contains one
    // stored BRR object with many references, not eight duplicate samples.
    for (std::size_t voice = 0; voice < 8; ++voice)
        bytes[spc_dsp_offset + voice * 0x10 + 0x04] = static_cast<std::uint8_t>((voice & 1u) == 0 ? 3 : 4);
    bytes[spc_dsp_offset + 0x5D] = 0x20; // DIR = $2000.

    const std::size_t dir3 = spc_ram_offset + 0x200C;
    const std::size_t dir4 = spc_ram_offset + 0x2010;
    for (const std::size_t entry : {dir3, dir4}) {
        bytes[entry + 0] = 0x56;
        bytes[entry + 1] = 0x34; // sample start $3456
        bytes[entry + 2] = 0x5F;
        bytes[entry + 3] = 0x34; // loop start $345F, second block
    }

    // Two 9-byte BRR blocks. The second block has END + LOOP.
    const std::size_t sample = spc_ram_offset + 0x3456;
    bytes[sample + 0] = 0x00;
    for (std::size_t i = 1; i < 9; ++i)
        bytes[sample + i] = static_cast<std::uint8_t>(0x10 + i);
    bytes[sample + 9] = 0x03;
    for (std::size_t i = 10; i < 18; ++i)
        bytes[sample + i] = static_cast<std::uint8_t>(0x20 + i);

    return bytes;
}

int main() {
    const auto bytes = make_fixture();
    const auto snapshot = parse_spc_snapshot(bytes);

    const auto scan = scan_brr_sample(snapshot, 0x3456, 0x345F);
    CHECK(scan.start_address == 0x3456);
    CHECK(scan.loop_address == 0x345F);
    CHECK(scan.end_block_address == 0x345F);
    CHECK(scan.block_count == 2);
    CHECK(scan.byte_count == 18);
    CHECK(scan.terminated);
    CHECK(scan.end_block_loops);
    CHECK(!scan.address_wrapped);
    CHECK(scan.compressed_bytes.size() == 18);
    CHECK(scan.compressed_bytes[0] == 0x00);
    CHECK(scan.compressed_bytes[9] == 0x03);

    musical_execution_graph graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        graph,
        snapshot,
        "shared-sample.spc",
        to_flags(provenance_flag::runtime_capture));
    const auto samples = materialize_spc_brr_samples(
        graph,
        snapshot,
        snapshot_graph,
        "shared-sample.spc",
        to_flags(provenance_flag::runtime_capture));

    CHECK(samples.sample_ids.size() == 1);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 1);
    const node_id sample_id = samples.sample_ids[0];
    CHECK(samples.voice_sample_ids[0].has_value());
    CHECK(samples.voice_sample_ids[1].has_value());
    CHECK(*samples.voice_sample_ids[0] == sample_id);
    CHECK(*samples.voice_sample_ids[1] == sample_id);

    const node* sample_node = graph.find_node(sample_id);
    CHECK(sample_node != nullptr);
    CHECK(sample_node->kind == node_kind::sample_buffer);
    CHECK(sample_node->layer == semantic_layer::synthesis);
    CHECK(std::get<std::string>(find_attribute(sample_node, "encoding")->value) == "BRR");
    CHECK(std::get<std::string>(find_attribute(sample_node, "identity_scope")->value) == "snapshot_ram_object");
    CHECK(std::get<std::uint64_t>(find_attribute(sample_node, "start_address")->value) == 0x3456);
    CHECK(std::get<std::uint64_t>(find_attribute(sample_node, "directory_loop_address")->value) == 0x345F);
    CHECK(std::get<std::uint64_t>(find_attribute(sample_node, "block_count")->value) == 2);
    CHECK(std::get<std::uint64_t>(find_attribute(sample_node, "compressed_byte_count")->value) == 18);
    CHECK(std::get<bool>(find_attribute(sample_node, "terminated")->value));
    CHECK(std::get<bool>(find_attribute(sample_node, "end_block_loops")->value));
    CHECK(!std::get<bool>(find_attribute(sample_node, "address_wrapped")->value));
    CHECK(std::get<std::string>(find_attribute(sample_node, "extent_status")->value) == "complete_to_brr_end");
    CHECK(std::get<std::string>(find_attribute(sample_node, "persistent_instrument_identity")->value) == "unresolved");

    // `references` is deliberately non-owning and non-identifying. Every saved
    // slot points at the shared stored sample while remaining a separate slot.
    for (std::size_t voice = 0; voice < 8; ++voice) {
        const auto refs = graph.edges_from(snapshot_graph.voice_slot_ids[voice], edge_kind::references);
        CHECK(refs.size() == 1);
        CHECK(refs[0]->to == sample_id);
        CHECK(std::get<std::string>(refs[0]->attributes[0].value) == "sample_source");
        CHECK(std::get<std::uint64_t>(refs[0]->attributes[1].value) == ((voice & 1u) == 0 ? 3u : 4u));
    }
    CHECK(graph.edges_to(sample_id, edge_kind::references).size() == 8);

    // Shared sample storage still does not prove one instrument, one live voice,
    // one musical part, or one note.
    CHECK(graph.nodes_of_kind(node_kind::instrument_definition).empty());
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());

    // BRR blocks may cross the 16-bit RAM boundary. The scanner preserves that
    // source geometry rather than treating wrap as an error or linearizing it
    // into an invented larger address space.
    auto wrapped_snapshot = snapshot;
    wrapped_snapshot.ram[0xFFFC] = 0x00;
    wrapped_snapshot.ram[0xFFFD] = 0x11;
    wrapped_snapshot.ram[0xFFFE] = 0x12;
    wrapped_snapshot.ram[0xFFFF] = 0x13;
    wrapped_snapshot.ram[0x0000] = 0x14;
    wrapped_snapshot.ram[0x0001] = 0x15;
    wrapped_snapshot.ram[0x0002] = 0x16;
    wrapped_snapshot.ram[0x0003] = 0x17;
    wrapped_snapshot.ram[0x0004] = 0x18;
    wrapped_snapshot.ram[0x0005] = 0x01;
    for (std::size_t i = 1; i < 9; ++i)
        wrapped_snapshot.ram[0x0005 + i] = static_cast<std::uint8_t>(0x30 + i);
    const auto wrapped = scan_brr_sample(wrapped_snapshot, 0xFFFC, 0x0005);
    CHECK(wrapped.terminated);
    CHECK(wrapped.block_count == 2);
    CHECK(wrapped.byte_count == 18);
    CHECK(wrapped.address_wrapped);
    CHECK(wrapped.end_block_address == 0x0005);

    // Malformed or non-terminating data stays bounded. Absence of END inside the
    // declared work bound is reported as uncertainty about extent, not silently
    // accepted as a complete sample.
    spc_snapshot unterminated{};
    unterminated.source_size = spc_min_file_size;
    const auto bounded = scan_brr_sample(unterminated, 0x0000, 0x0000);
    CHECK(!bounded.terminated);
    CHECK(!bounded.end_block_loops);
    CHECK(bounded.block_count == brr_max_scan_blocks);
    CHECK(bounded.byte_count == brr_max_scan_blocks * brr_block_size);
    CHECK(bounded.byte_count <= brr_max_scan_bytes);

    return 0;
}
