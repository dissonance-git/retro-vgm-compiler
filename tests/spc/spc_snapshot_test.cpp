#include "../../components/spc/spc_snapshot_graph_adapter.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
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

    bytes[0x23] = 0x1A; // ID666 present marker, preserved raw.
    bytes[0x24] = 30;
    bytes[0x25] = 0x34;
    bytes[0x26] = 0x12;
    bytes[0x27] = 0xA1;
    bytes[0x28] = 0xB2;
    bytes[0x29] = 0xC3;
    bytes[0x2A] = 0xD4;
    bytes[0x2B] = 0xEF;
    bytes[0x2C] = 'T';
    bytes[0x2D] = 'E';
    bytes[0x2E] = 'S';
    bytes[0x2F] = 'T';

    // Voice 0 saved register image.
    bytes[spc_dsp_offset + 0x00] = 0x80;
    bytes[spc_dsp_offset + 0x01] = 0x7F;
    bytes[spc_dsp_offset + 0x02] = 0x34;
    bytes[spc_dsp_offset + 0x03] = 0x12;
    bytes[spc_dsp_offset + 0x04] = 0x03;
    bytes[spc_dsp_offset + 0x05] = 0x8F;
    bytes[spc_dsp_offset + 0x06] = 0xE0;
    bytes[spc_dsp_offset + 0x07] = 0x7F;
    bytes[spc_dsp_offset + 0x08] = 0x55;
    bytes[spc_dsp_offset + 0x09] = 0xAA;

    // Voice 1 gets a distinct pitch and is flagged for pitch modulation.
    bytes[spc_dsp_offset + 0x12] = 0x78;
    bytes[spc_dsp_offset + 0x13] = 0x05;
    bytes[spc_dsp_offset + 0x14] = 0x04;

    // Global S-DSP register image.
    bytes[spc_dsp_offset + 0x0C] = 0x81;
    bytes[spc_dsp_offset + 0x1C] = 0x7E;
    bytes[spc_dsp_offset + 0x2C] = 0x90;
    bytes[spc_dsp_offset + 0x3C] = 0x70;
    bytes[spc_dsp_offset + 0x4C] = 0x01;
    bytes[spc_dsp_offset + 0x5C] = 0x00;
    bytes[spc_dsp_offset + 0x6C] = 0x1F;
    bytes[spc_dsp_offset + 0x7C] = 0x04;
    bytes[spc_dsp_offset + 0x0D] = 0xF0;
    bytes[spc_dsp_offset + 0x2D] = 0x02;
    bytes[spc_dsp_offset + 0x3D] = 0x01;
    bytes[spc_dsp_offset + 0x4D] = 0x01;
    bytes[spc_dsp_offset + 0x5D] = 0x20;
    bytes[spc_dsp_offset + 0x6D] = 0x40;
    bytes[spc_dsp_offset + 0x7D] = 0x0A;
    for (std::size_t tap = 0; tap < 8; ++tap)
        bytes[spc_dsp_offset + 0x0F + tap * 0x10] = static_cast<std::uint8_t>(tap == 0 ? 0xF8 : tap);

    // DIR=$20, SRCN=$03 -> directory entry $200C.
    const std::size_t directory_entry = spc_ram_offset + 0x200C;
    bytes[directory_entry + 0] = 0x56;
    bytes[directory_entry + 1] = 0x34;
    bytes[directory_entry + 2] = 0x70;
    bytes[directory_entry + 3] = 0x34;

    // Distinct trailer bytes prove that the full optional tail is preserved.
    bytes[spc_trailing_offset] = 0x5A;
    bytes[spc_ipl_offset] = 0xCD;
    bytes[spc_ipl_offset + 0x3F] = 0xEF;
    return bytes;
}

int main() {
    const auto bytes = make_fixture();
    const auto snapshot = parse_spc_snapshot(bytes);

    CHECK(snapshot.source_size == spc_full_file_size);
    CHECK(snapshot.has_id666 == 0x1A);
    CHECK(snapshot.version == 30);
    CHECK(snapshot.cpu.pc == 0x1234);
    CHECK(snapshot.cpu.a == 0xA1);
    CHECK(snapshot.cpu.x == 0xB2);
    CHECK(snapshot.cpu.y == 0xC3);
    CHECK(snapshot.cpu.psw == 0xD4);
    CHECK(snapshot.cpu.sp == 0xEF);
    CHECK(snapshot.header_payload[0] == 'T');
    CHECK(snapshot.ram[0x200C] == 0x56);
    CHECK(snapshot.dsp[0x04] == 0x03);
    CHECK(snapshot.has_trailing_state);
    CHECK(snapshot.trailing_unused[0] == 0x5A);
    CHECK(snapshot.ipl_rom[0] == 0xCD);
    CHECK(snapshot.ipl_rom[0x3F] == 0xEF);

    // The minimum standard image remains valid and simply lacks the optional
    // trailing 0x80-byte state.
    const auto minimum = parse_spc_snapshot(bytes.data(), spc_min_file_size);
    CHECK(minimum.source_size == spc_min_file_size);
    CHECK(!minimum.has_trailing_state);
    CHECK(minimum.cpu.pc == 0x1234);
    CHECK(minimum.dsp[0x5D] == 0x20);

    bool rejected_bad_signature = false;
    try {
        auto bad = bytes;
        bad[0] = 'X';
        (void)parse_spc_snapshot(bad);
    } catch (const std::invalid_argument&) {
        rejected_bad_signature = true;
    }
    CHECK(rejected_bad_signature);

    bool rejected_truncated = false;
    try {
        (void)parse_spc_snapshot(bytes.data(), spc_min_file_size - 1);
    } catch (const std::invalid_argument&) {
        rejected_truncated = true;
    }
    CHECK(rejected_truncated);

    musical_execution_graph graph;
    const auto materialized = materialize_spc_snapshot(
        graph,
        snapshot,
        "fixture.spc",
        to_flags(provenance_flag::runtime_capture));

    const node* source = graph.find_node(materialized.snapshot_id);
    const node* dsp = graph.find_node(materialized.dsp_register_image_id);
    CHECK(source != nullptr);
    CHECK(dsp != nullptr);
    CHECK(source->kind == node_kind::source_object);
    CHECK(source->layer == semantic_layer::source_representation);
    CHECK(dsp->kind == node_kind::synthesis_object);
    CHECK(dsp->layer == semantic_layer::synthesis);
    CHECK(dsp->provenance[0].status == evidence_status::exact);
    CHECK(dsp->provenance[0].byte_offset.has_value());
    CHECK(*dsp->provenance[0].byte_offset == spc_dsp_offset);

    CHECK(std::get<std::int64_t>(find_attribute(dsp, "master_volume_left")->value) == -127);
    CHECK(std::get<std::int64_t>(find_attribute(dsp, "echo_feedback")->value) == -16);
    CHECK(std::get<std::uint64_t>(find_attribute(dsp, "sample_directory_page")->value) == 0x20);
    CHECK(std::get<std::uint64_t>(find_attribute(dsp, "pitch_modulation_mask")->value) == 0x02);
    CHECK(std::get<std::int64_t>(find_attribute(dsp, "fir_0")->value) == -8);
    CHECK(std::get<std::int64_t>(find_attribute(dsp, "fir_7")->value) == 7);

    const auto source_contents = graph.edges_from(materialized.snapshot_id, edge_kind::contains);
    CHECK(source_contents.size() == 1);
    CHECK(source_contents[0]->to == materialized.dsp_register_image_id);

    const auto dsp_contents = graph.edges_from(materialized.dsp_register_image_id, edge_kind::contains);
    CHECK(dsp_contents.size() == 8);
    CHECK(graph.nodes_of_kind(node_kind::physical_slot).size() == 8);

    const node* voice0 = graph.find_node(materialized.voice_slot_ids[0]);
    const node* voice1 = graph.find_node(materialized.voice_slot_ids[1]);
    CHECK(voice0 != nullptr);
    CHECK(voice1 != nullptr);
    CHECK(voice0->kind == node_kind::physical_slot);
    CHECK(voice0->layer == semantic_layer::synthesis);
    CHECK(std::get<std::int64_t>(find_attribute(voice0, "volume_left")->value) == -128);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "pitch_code")->value) == 0x1234);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "source_index")->value) == 0x03);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "sample_directory_base")->value) == 0x2000);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "sample_directory_entry")->value) == 0x200C);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "sample_start")->value) == 0x3456);
    CHECK(std::get<std::uint64_t>(find_attribute(voice0, "sample_loop_start")->value) == 0x3470);
    CHECK(std::get<bool>(find_attribute(voice0, "saved_kon_bit")->value));
    CHECK(std::get<bool>(find_attribute(voice0, "noise_enabled")->value));
    CHECK(std::get<bool>(find_attribute(voice0, "echo_enabled")->value));
    CHECK(std::get<std::string>(find_attribute(voice0, "live_voice_microstate")->value) == "not_in_spc_register_image");
    CHECK(std::get<std::string>(find_attribute(voice0, "persistent_part_identity")->value) == "unresolved");
    CHECK(std::get<bool>(find_attribute(voice1, "pitch_modulated")->value));

    // This first snapshot slice must not hallucinate a live sounding episode,
    // musical note, persistent part, or MIDI/notation projection from register
    // bytes that do not contain the DSP's hidden runtime phase.
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).empty());
    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());

    return 0;
}
