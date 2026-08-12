#pragma once

#include "spc_snapshot_graph_adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gameaudio::spc {

constexpr std::size_t brr_block_size = 9;
constexpr std::size_t brr_payload_size = 8;
constexpr std::size_t brr_max_scan_bytes = spc_ram_size;
constexpr std::size_t brr_max_scan_blocks = brr_max_scan_bytes / brr_block_size;

struct brr_sample_scan {
    std::uint16_t start_address = 0;
    std::uint16_t end_block_address = 0;
    std::size_t block_count = 0;
    std::size_t byte_count = 0;
    bool terminated = false;
    bool end_block_loops = false;
    bool address_wrapped = false;
    std::vector<std::uint8_t> compressed_bytes;
};

inline std::uint8_t spc_ram_byte(const std::uint8_t* ram, std::uint32_t address) noexcept {
    return ram[static_cast<std::uint16_t>(address)];
}

inline std::uint8_t spc_ram_byte(const spc_snapshot& snapshot, std::uint32_t address) noexcept {
    return spc_ram_byte(snapshot.ram.data(), address);
}

inline brr_sample_scan scan_brr_sample(
    const std::uint8_t* ram,
    std::uint16_t start_address) {
    brr_sample_scan result;
    result.start_address = start_address;
    result.end_block_address = start_address;
    result.compressed_bytes.reserve(brr_max_scan_bytes);

    if (ram == nullptr)
        return result;

    std::uint16_t block_address = start_address;
    for (std::size_t block = 0; block < brr_max_scan_blocks; ++block) {
        result.end_block_address = block_address;
        const std::uint8_t header = spc_ram_byte(ram, block_address);

        for (std::size_t byte = 0; byte < brr_block_size; ++byte) {
            const std::uint32_t absolute = static_cast<std::uint32_t>(block_address) +
                static_cast<std::uint32_t>(byte);
            if (absolute > 0xFFFFu)
                result.address_wrapped = true;
            result.compressed_bytes.push_back(spc_ram_byte(ram, absolute));
        }

        ++result.block_count;
        result.byte_count += brr_block_size;

        if ((header & 0x01u) != 0) {
            result.terminated = true;
            result.end_block_loops = (header & 0x02u) != 0;
            break;
        }

        const std::uint32_t next = static_cast<std::uint32_t>(block_address) + brr_block_size;
        if (next > 0xFFFFu)
            result.address_wrapped = true;
        block_address = static_cast<std::uint16_t>(next);
    }

    return result;
}

inline brr_sample_scan scan_brr_sample(
    const spc_snapshot& snapshot,
    std::uint16_t start_address) {
    return scan_brr_sample(snapshot.ram.data(), start_address);
}

struct spc_brr_sample_graph_handle {
    spc_snapshot_graph_handle snapshot;
    std::array<std::optional<vgmtooling::model::node_id>, 8> voice_sample_ids{};
    std::vector<vgmtooling::model::node_id> sample_ids;
};

inline spc_brr_sample_graph_handle materialize_spc_brr_samples(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_snapshot& snapshot,
    const spc_snapshot_graph_handle& snapshot_graph,
    std::string source,
    vgmtooling::model::provenance_flags flags =
        vgmtooling::model::to_flags(vgmtooling::model::provenance_flag::none)) {
    using namespace vgmtooling::model;

    spc_brr_sample_graph_handle handle;
    handle.snapshot = snapshot_graph;

    const std::uint16_t directory_base = static_cast<std::uint16_t>(snapshot.dsp[0x5D] << 8u);
    std::unordered_map<std::uint16_t, node_id> samples_by_start;

    for (std::size_t voice = 0; voice < 8; ++voice) {
        const std::size_t voice_base = voice * 0x10;
        const std::uint8_t srcn = snapshot.dsp[voice_base + 0x04];
        const std::uint16_t directory_entry = static_cast<std::uint16_t>(
            directory_base + static_cast<std::uint16_t>(srcn) * 4u);
        const std::uint16_t sample_start = spc_ram_le16(snapshot, directory_entry);
        const std::uint16_t loop_start = spc_ram_le16(
            snapshot,
            static_cast<std::uint16_t>(directory_entry + 2u));

        node_id sample_id = 0;
        const auto existing = samples_by_start.find(sample_start);
        if (existing != samples_by_start.end()) {
            sample_id = existing->second;
        } else {
            const brr_sample_scan scan = scan_brr_sample(snapshot, sample_start);

            node sample;
            sample.kind = node_kind::sample_buffer;
            sample.layer = semantic_layer::synthesis;
            sample.flow = flow_kind::value;
            sample.label = "BRR sample object in SPC RAM";
            sample.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
            sample.attributes.push_back({"identity_scope", std::string{"snapshot_ram_object"}, evidence_status::derived, 1.0, ""});
            sample.attributes.push_back({"start_address", static_cast<std::uint64_t>(scan.start_address), evidence_status::derived, 1.0, "address"});
            sample.attributes.push_back({"end_block_address", static_cast<std::uint64_t>(scan.end_block_address), evidence_status::derived, 1.0, "address"});
            sample.attributes.push_back({"block_count", static_cast<std::uint64_t>(scan.block_count), evidence_status::derived, 1.0, "blocks"});
            sample.attributes.push_back({"compressed_byte_count", static_cast<std::uint64_t>(scan.byte_count), evidence_status::derived, 1.0, "bytes"});
            sample.attributes.push_back({"terminated", scan.terminated, evidence_status::derived, 1.0, ""});
            sample.attributes.push_back({"end_block_loops", scan.end_block_loops, evidence_status::derived, 1.0, ""});
            sample.attributes.push_back({"address_wrapped", scan.address_wrapped, evidence_status::derived, 1.0, ""});
            sample.attributes.push_back({
                "extent_status",
                std::string{scan.terminated ? "complete_to_brr_end" : "bounded_no_end"},
                evidence_status::derived,
                1.0,
                "",
            });
            sample.attributes.push_back({"persistent_instrument_identity", std::string{"unresolved"}, evidence_status::derived, 1.0, ""});
            sample.provenance.push_back({
                evidence_status::derived,
                1.0,
                source,
                static_cast<std::uint64_t>(spc_ram_offset + sample_start),
                scan.terminated
                    ? "BRR extent scanned deterministically from exact SPC RAM until END flag"
                    : "BRR scan reached the bounded one-RAM-image work limit without an END flag",
                flags,
            });
            sample_id = graph.add_node(std::move(sample));
            samples_by_start.emplace(sample_start, sample_id);
            handle.sample_ids.push_back(sample_id);
        }

        handle.voice_sample_ids[voice] = sample_id;

        edge reference;
        reference.kind = edge_kind::references;
        reference.from = snapshot_graph.voice_slot_ids[voice];
        reference.to = sample_id;
        reference.attributes.push_back({"reference_kind", std::string{"sample_source"}, evidence_status::derived, 1.0, ""});
        reference.attributes.push_back({"source_index", static_cast<std::uint64_t>(srcn), evidence_status::exact, 1.0, "slot"});
        reference.attributes.push_back({"directory_entry", static_cast<std::uint64_t>(directory_entry), evidence_status::derived, 1.0, "address"});
        reference.attributes.push_back({"directory_loop_address", static_cast<std::uint64_t>(loop_start), evidence_status::derived, 1.0, "address"});
        reference.provenance.push_back({
            evidence_status::derived,
            1.0,
            source,
            static_cast<std::uint64_t>(spc_dsp_offset + voice_base + 0x04),
            "saved SRCN resolves through exact SPC RAM to this snapshot-local BRR object; the directory loop target remains reference-specific",
            flags,
        });
        graph.add_edge(std::move(reference));
    }

    return handle;
}

} // namespace gameaudio::spc
