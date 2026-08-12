#pragma once

#include "spc_snapshot.h"
#include "../../model/musical_execution_graph.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace gameaudio::spc {

struct spc_snapshot_graph_handle {
    vgmtooling::model::node_id snapshot_id = 0;
    vgmtooling::model::node_id dsp_register_image_id = 0;
    std::array<vgmtooling::model::node_id, 8> voice_slot_ids{};
};

inline std::uint16_t spc_ram_le16(const spc_snapshot& snapshot, std::uint16_t address) noexcept {
    const std::uint16_t next = static_cast<std::uint16_t>(address + 1u);
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(snapshot.ram[address]) |
        (static_cast<std::uint16_t>(snapshot.ram[next]) << 8u));
}

inline std::int64_t spc_signed8(std::uint8_t value) noexcept {
    return static_cast<std::int64_t>(static_cast<std::int8_t>(value));
}

inline spc_snapshot_graph_handle materialize_spc_snapshot(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_snapshot& snapshot,
    std::string source,
    vgmtooling::model::provenance_flags flags =
        vgmtooling::model::to_flags(vgmtooling::model::provenance_flag::none)) {
    using namespace vgmtooling::model;

    spc_snapshot_graph_handle handle;

    node source_node;
    source_node.kind = node_kind::source_object;
    source_node.layer = semantic_layer::source_representation;
    source_node.flow = flow_kind::value;
    source_node.label = "SNES-SPC700 machine snapshot";
    source_node.attributes.push_back({"format", std::string{"SPC"}, evidence_status::exact, 1.0, ""});
    source_node.attributes.push_back({"file_size", static_cast<std::uint64_t>(snapshot.source_size), evidence_status::exact, 1.0, "bytes"});
    source_node.attributes.push_back({"has_id666", static_cast<std::uint64_t>(snapshot.has_id666), evidence_status::exact, 1.0, ""});
    source_node.attributes.push_back({"version", static_cast<std::uint64_t>(snapshot.version), evidence_status::exact, 1.0, ""});
    source_node.attributes.push_back({"cpu_pc", static_cast<std::uint64_t>(snapshot.cpu.pc), evidence_status::exact, 1.0, "address"});
    source_node.attributes.push_back({"cpu_a", static_cast<std::uint64_t>(snapshot.cpu.a), evidence_status::exact, 1.0, "byte"});
    source_node.attributes.push_back({"cpu_x", static_cast<std::uint64_t>(snapshot.cpu.x), evidence_status::exact, 1.0, "byte"});
    source_node.attributes.push_back({"cpu_y", static_cast<std::uint64_t>(snapshot.cpu.y), evidence_status::exact, 1.0, "byte"});
    source_node.attributes.push_back({"cpu_psw", static_cast<std::uint64_t>(snapshot.cpu.psw), evidence_status::exact, 1.0, "byte"});
    source_node.attributes.push_back({"cpu_sp", static_cast<std::uint64_t>(snapshot.cpu.sp), evidence_status::exact, 1.0, "byte"});
    source_node.attributes.push_back({"has_trailing_state", snapshot.has_trailing_state, evidence_status::exact, 1.0, ""});
    source_node.provenance.push_back({
        evidence_status::exact,
        1.0,
        source,
        0u,
        "exact SPC file image",
        flags,
    });
    handle.snapshot_id = graph.add_node(std::move(source_node));

    node dsp;
    dsp.kind = node_kind::synthesis_object;
    dsp.layer = semantic_layer::synthesis;
    dsp.flow = flow_kind::value;
    dsp.label = "S-DSP saved register image";
    dsp.attributes.push_back({"register_count", static_cast<std::uint64_t>(spc_dsp_size), evidence_status::exact, 1.0, "bytes"});
    dsp.attributes.push_back({"master_volume_left", spc_signed8(snapshot.dsp[0x0C]), evidence_status::exact, 1.0, "register"});
    dsp.attributes.push_back({"master_volume_right", spc_signed8(snapshot.dsp[0x1C]), evidence_status::exact, 1.0, "register"});
    dsp.attributes.push_back({"echo_volume_left", spc_signed8(snapshot.dsp[0x2C]), evidence_status::exact, 1.0, "register"});
    dsp.attributes.push_back({"echo_volume_right", spc_signed8(snapshot.dsp[0x3C]), evidence_status::exact, 1.0, "register"});
    dsp.attributes.push_back({"saved_kon_mask", static_cast<std::uint64_t>(snapshot.dsp[0x4C]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"saved_koff_mask", static_cast<std::uint64_t>(snapshot.dsp[0x5C]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"flags", static_cast<std::uint64_t>(snapshot.dsp[0x6C]), evidence_status::exact, 1.0, "byte"});
    dsp.attributes.push_back({"endx", static_cast<std::uint64_t>(snapshot.dsp[0x7C]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"echo_feedback", spc_signed8(snapshot.dsp[0x0D]), evidence_status::exact, 1.0, "register"});
    dsp.attributes.push_back({"pitch_modulation_mask", static_cast<std::uint64_t>(snapshot.dsp[0x2D]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"noise_mask", static_cast<std::uint64_t>(snapshot.dsp[0x3D]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"echo_enable_mask", static_cast<std::uint64_t>(snapshot.dsp[0x4D]), evidence_status::exact, 1.0, "mask"});
    dsp.attributes.push_back({"sample_directory_page", static_cast<std::uint64_t>(snapshot.dsp[0x5D]), evidence_status::exact, 1.0, "page"});
    dsp.attributes.push_back({"echo_start_page", static_cast<std::uint64_t>(snapshot.dsp[0x6D]), evidence_status::exact, 1.0, "page"});
    dsp.attributes.push_back({"echo_delay", static_cast<std::uint64_t>(snapshot.dsp[0x7D] & 0x0Fu), evidence_status::derived, 1.0, "register"});
    for (std::size_t tap = 0; tap < 8; ++tap) {
        dsp.attributes.push_back({
            std::string{"fir_"} + std::to_string(tap),
            spc_signed8(snapshot.dsp[0x0F + tap * 0x10]),
            evidence_status::exact,
            1.0,
            "coefficient",
        });
    }
    dsp.provenance.push_back({
        evidence_status::exact,
        1.0,
        source,
        spc_dsp_offset,
        "exact 128-byte S-DSP register image saved in SPC file; hidden live DSP microstate is not asserted",
        flags,
    });
    handle.dsp_register_image_id = graph.add_node(std::move(dsp));

    edge source_contains_dsp;
    source_contains_dsp.kind = edge_kind::contains;
    source_contains_dsp.from = handle.snapshot_id;
    source_contains_dsp.to = handle.dsp_register_image_id;
    source_contains_dsp.provenance.push_back({
        evidence_status::exact,
        1.0,
        source,
        spc_dsp_offset,
        "SPC file contains this saved DSP register image",
        flags,
    });
    graph.add_edge(std::move(source_contains_dsp));

    const std::uint16_t directory_base = static_cast<std::uint16_t>(snapshot.dsp[0x5D] << 8u);

    for (std::size_t voice = 0; voice < 8; ++voice) {
        const std::size_t base = voice * 0x10;
        const std::uint16_t pitch = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(snapshot.dsp[base + 0x02]) |
            ((static_cast<std::uint16_t>(snapshot.dsp[base + 0x03]) & 0x3Fu) << 8u));
        const std::uint8_t srcn = snapshot.dsp[base + 0x04];
        const std::uint16_t directory_entry = static_cast<std::uint16_t>(
            directory_base + static_cast<std::uint16_t>(srcn) * 4u);
        const std::uint16_t sample_start = spc_ram_le16(snapshot, directory_entry);
        const std::uint16_t loop_start = spc_ram_le16(
            snapshot,
            static_cast<std::uint16_t>(directory_entry + 2u));
        const std::uint8_t voice_bit = static_cast<std::uint8_t>(1u << voice);

        node slot;
        slot.kind = node_kind::physical_slot;
        slot.layer = semantic_layer::synthesis;
        slot.flow = flow_kind::value;
        slot.label = std::string{"S-DSP saved voice-register slot "} + std::to_string(voice);
        slot.attributes.push_back({"physical_slot", static_cast<std::uint64_t>(voice), evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"volume_left", spc_signed8(snapshot.dsp[base + 0x00]), evidence_status::exact, 1.0, "register"});
        slot.attributes.push_back({"volume_right", spc_signed8(snapshot.dsp[base + 0x01]), evidence_status::exact, 1.0, "register"});
        slot.attributes.push_back({"pitch_code", static_cast<std::uint64_t>(pitch), evidence_status::derived, 1.0, "device_native"});
        slot.attributes.push_back({"source_index", static_cast<std::uint64_t>(srcn), evidence_status::exact, 1.0, "slot"});
        slot.attributes.push_back({"adsr0", static_cast<std::uint64_t>(snapshot.dsp[base + 0x05]), evidence_status::exact, 1.0, "byte"});
        slot.attributes.push_back({"adsr1", static_cast<std::uint64_t>(snapshot.dsp[base + 0x06]), evidence_status::exact, 1.0, "byte"});
        slot.attributes.push_back({"gain", static_cast<std::uint64_t>(snapshot.dsp[base + 0x07]), evidence_status::exact, 1.0, "byte"});
        slot.attributes.push_back({"saved_envx", static_cast<std::uint64_t>(snapshot.dsp[base + 0x08]), evidence_status::exact, 1.0, "register"});
        slot.attributes.push_back({"saved_outx", static_cast<std::uint64_t>(snapshot.dsp[base + 0x09]), evidence_status::exact, 1.0, "register"});
        slot.attributes.push_back({"sample_directory_base", static_cast<std::uint64_t>(directory_base), evidence_status::derived, 1.0, "address"});
        slot.attributes.push_back({"sample_directory_entry", static_cast<std::uint64_t>(directory_entry), evidence_status::derived, 1.0, "address"});
        slot.attributes.push_back({"sample_start", static_cast<std::uint64_t>(sample_start), evidence_status::derived, 1.0, "address"});
        slot.attributes.push_back({"sample_loop_start", static_cast<std::uint64_t>(loop_start), evidence_status::derived, 1.0, "address"});
        slot.attributes.push_back({"saved_kon_bit", (snapshot.dsp[0x4C] & voice_bit) != 0, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"saved_koff_bit", (snapshot.dsp[0x5C] & voice_bit) != 0, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"pitch_modulated", (snapshot.dsp[0x2D] & voice_bit) != 0, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"noise_enabled", (snapshot.dsp[0x3D] & voice_bit) != 0, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"echo_enabled", (snapshot.dsp[0x4D] & voice_bit) != 0, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"live_voice_microstate", std::string{"not_in_spc_register_image"}, evidence_status::exact, 1.0, ""});
        slot.attributes.push_back({"persistent_part_identity", std::string{"unresolved"}, evidence_status::exact, 1.0, ""});
        slot.provenance.push_back({
            evidence_status::exact,
            1.0,
            source,
            static_cast<std::uint64_t>(spc_dsp_offset + base),
            "saved S-DSP voice register block plus deterministic directory lookup into exact SPC RAM; not a live voice episode",
            flags,
        });
        handle.voice_slot_ids[voice] = graph.add_node(std::move(slot));

        edge dsp_contains_slot;
        dsp_contains_slot.kind = edge_kind::contains;
        dsp_contains_slot.from = handle.dsp_register_image_id;
        dsp_contains_slot.to = handle.voice_slot_ids[voice];
        dsp_contains_slot.provenance.push_back({
            evidence_status::exact,
            1.0,
            source,
            static_cast<std::uint64_t>(spc_dsp_offset + base),
            "voice register block belongs to saved S-DSP register image",
            flags,
        });
        graph.add_edge(std::move(dsp_contains_slot));
    }

    return handle;
}

} // namespace gameaudio::spc
