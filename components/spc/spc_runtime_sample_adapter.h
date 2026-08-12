#pragma once

#include "spc_brr_sample.h"
#include "spc_ram_shadow.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::spc {

enum class spc_runtime_sample_status : std::uint8_t {
    materialized = 0,
    reused,
    shadow_not_synchronized,
    ram_changed_after_observation,
};

inline const char* spc_runtime_sample_status_name(spc_runtime_sample_status status) noexcept {
    switch (status) {
    case spc_runtime_sample_status::materialized:
        return "materialized";
    case spc_runtime_sample_status::reused:
        return "reused";
    case spc_runtime_sample_status::shadow_not_synchronized:
        return "shadow_not_synchronized";
    case spc_runtime_sample_status::ram_changed_after_observation:
        return "ram_changed_after_observation";
    }
    return "unknown";
}

struct spc_runtime_sample_observation {
    vgmtooling::model::node_id trace_event_id = 0;
    std::optional<vgmtooling::model::node_id> physical_voice_episode_id{};
    std::uint8_t source_index = 0;
    std::uint16_t start_address = 0;
    std::optional<std::uint16_t> directory_loop_address{};
    std::uint64_t ram_write_serial = 0;
    std::int64_t tick = 0;
    std::uint64_t tick_rate = 0;
};

struct spc_runtime_sample_cache_entry {
    vgmtooling::model::node_id sample_id = 0;
    brr_sample_scan scan;
    spc_ram_extent_stamp extent_stamp;
};

struct spc_runtime_sample_graph_handle {
    std::string source;
    vgmtooling::model::provenance_flags provenance_flags =
        vgmtooling::model::to_flags(vgmtooling::model::provenance_flag::none);
    std::vector<spc_runtime_sample_cache_entry> cache;
};

struct spc_runtime_sample_result {
    spc_runtime_sample_status status = spc_runtime_sample_status::shadow_not_synchronized;
    std::optional<vgmtooling::model::node_id> sample_id{};
    std::optional<vgmtooling::model::edge_id> reference_edge_id{};
};

inline spc_runtime_sample_graph_handle begin_spc_runtime_sample_graph(
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    spc_runtime_sample_graph_handle handle;
    handle.source = std::move(source);
    handle.provenance_flags = flags;
    return handle;
}

inline void annotate_spc_runtime_sample_status(
    vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id trace_event_id,
    spc_runtime_sample_status status) {
    using namespace vgmtooling::model;
    node* event = graph.find_node(trace_event_id);
    if (event == nullptr)
        throw std::invalid_argument("runtime sample observation references an unknown trace event");
    event->attributes.push_back({
        "runtime_sample_materialization",
        std::string{spc_runtime_sample_status_name(status)},
        evidence_status::derived,
        1.0,
        "",
    });
}

inline vgmtooling::model::edge_id add_spc_runtime_sample_reference(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_runtime_sample_graph_handle& handle,
    const spc_runtime_sample_observation& observation,
    vgmtooling::model::node_id sample_id) {
    using namespace vgmtooling::model;

    edge reference;
    reference.kind = edge_kind::references;
    reference.from = observation.trace_event_id;
    reference.to = sample_id;
    reference.attributes.push_back({
        "reference_kind",
        std::string{"runtime_sample_source"},
        evidence_status::derived,
        1.0,
        "",
    });
    reference.attributes.push_back({
        "source_index",
        static_cast<std::uint64_t>(observation.source_index),
        evidence_status::exact,
        1.0,
        "slot",
    });
    reference.attributes.push_back({
        "observation_tick",
        static_cast<std::int64_t>(observation.tick),
        evidence_status::exact,
        1.0,
        "device_tick",
    });
    reference.attributes.push_back({
        "observation_tick_rate",
        observation.tick_rate,
        evidence_status::exact,
        1.0,
        "ticks_per_second",
    });
    reference.attributes.push_back({
        "ram_write_serial",
        observation.ram_write_serial,
        evidence_status::exact,
        1.0,
        "write_sequence",
    });
    if (observation.directory_loop_address.has_value()) {
        reference.attributes.push_back({
            "directory_loop_address",
            static_cast<std::uint64_t>(*observation.directory_loop_address),
            evidence_status::exact,
            1.0,
            "address",
        });
    }
    reference.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        "runtime source observation references a BRR RAM version whose pages are proven unchanged between the event and synchronized analysis shadow",
        handle.provenance_flags | provenance_flag::runtime_capture,
    });
    return graph.add_edge(std::move(reference));
}

inline vgmtooling::model::node_id add_spc_runtime_sample_node(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_runtime_sample_graph_handle& handle,
    const spc_runtime_sample_observation& observation,
    const spc_ram_shadow& shadow,
    const brr_sample_scan& scan) {
    using namespace vgmtooling::model;

    node sample;
    sample.kind = node_kind::sample_buffer;
    sample.layer = semantic_layer::synthesis;
    sample.flow = flow_kind::value;
    sample.label = "BRR runtime RAM version";
    sample.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    sample.attributes.push_back({"identity_scope", std::string{"runtime_ram_version"}, evidence_status::derived, 1.0, ""});
    sample.attributes.push_back({"start_address", static_cast<std::uint64_t>(scan.start_address), evidence_status::exact, 1.0, "address"});
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
    sample.attributes.push_back({"first_observation_tick", static_cast<std::int64_t>(observation.tick), evidence_status::exact, 1.0, "device_tick"});
    sample.attributes.push_back({"first_observation_tick_rate", observation.tick_rate, evidence_status::exact, 1.0, "ticks_per_second"});
    sample.attributes.push_back({"event_ram_write_serial", observation.ram_write_serial, evidence_status::exact, 1.0, "write_sequence"});
    sample.attributes.push_back({"shadow_write_serial", shadow.synchronized_write_serial(), evidence_status::exact, 1.0, "write_sequence"});
    sample.attributes.push_back({"event_time_ram_exact", true, evidence_status::derived, 1.0, ""});
    sample.attributes.push_back({"persistent_instrument_identity", std::string{"unresolved"}, evidence_status::derived, 1.0, ""});

    provenance_flags node_flags = handle.provenance_flags | provenance_flag::runtime_capture;
    if (!scan.terminated)
        node_flags = node_flags | provenance_flag::incomplete;
    sample.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        scan.terminated
            ? "BRR runtime extent reconstructed from a synchronized APURAM shadow whose relevant pages did not change after the source event"
            : "bounded BRR runtime extent reconstructed from stable event-time APURAM pages, but no END block was observed within one RAM image",
        node_flags,
    });

    return graph.add_node(std::move(sample));
}

inline spc_runtime_sample_result materialize_spc_runtime_sample(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_sample_graph_handle& handle,
    const spc_ram_shadow& shadow,
    const spc_runtime_sample_observation& observation) {
    using namespace vgmtooling::model;

    if (graph.find_node(observation.trace_event_id) == nullptr)
        throw std::invalid_argument("runtime sample observation references an unknown trace event");
    if (observation.physical_voice_episode_id.has_value() &&
        graph.find_node(*observation.physical_voice_episode_id) == nullptr)
        throw std::invalid_argument("runtime sample observation references an unknown physical voice episode");

    spc_runtime_sample_result result;

    if (!shadow.initialized() || shadow.synchronized_write_serial() < observation.ram_write_serial) {
        result.status = spc_runtime_sample_status::shadow_not_synchronized;
        annotate_spc_runtime_sample_status(graph, observation.trace_event_id, result.status);
        return result;
    }

    const brr_sample_scan scan = scan_brr_sample(shadow.data(), observation.start_address);
    const spc_ram_extent_stamp current_extent = shadow.capture_extent(
        observation.start_address,
        scan.byte_count);

    if (!shadow.extent_represents_event_time(current_extent, observation.ram_write_serial)) {
        result.status = spc_runtime_sample_status::ram_changed_after_observation;
        annotate_spc_runtime_sample_status(graph, observation.trace_event_id, result.status);
        return result;
    }

    for (const auto& cached : handle.cache) {
        if (cached.scan.start_address != scan.start_address ||
            cached.scan.byte_count != scan.byte_count ||
            cached.scan.terminated != scan.terminated ||
            !shadow.extent_unchanged(cached.extent_stamp)) {
            continue;
        }

        result.status = spc_runtime_sample_status::reused;
        result.sample_id = cached.sample_id;
        result.reference_edge_id = add_spc_runtime_sample_reference(
            graph,
            handle,
            observation,
            cached.sample_id);
        annotate_spc_runtime_sample_status(graph, observation.trace_event_id, result.status);
        return result;
    }

    const node_id sample_id = add_spc_runtime_sample_node(
        graph,
        handle,
        observation,
        shadow,
        scan);

    spc_runtime_sample_cache_entry cached;
    cached.sample_id = sample_id;
    cached.scan = scan;
    cached.extent_stamp = current_extent;
    handle.cache.push_back(std::move(cached));

    result.status = spc_runtime_sample_status::materialized;
    result.sample_id = sample_id;
    result.reference_edge_id = add_spc_runtime_sample_reference(
        graph,
        handle,
        observation,
        sample_id);
    annotate_spc_runtime_sample_status(graph, observation.trace_event_id, result.status);
    return result;
}

} // namespace gameaudio::spc
