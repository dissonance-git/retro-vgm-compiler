#pragma once

#include "genesis_state.h"
#include "vgm_execution_graph_adapter.h"

#include <array>
#include <optional>
#include <string>
#include <utility>

namespace gameaudio::vgm {

struct genesis_execution_graph_handle {
    vgm_execution_trace_handle source_trace;
    genesis_state shadow;
    std::array<std::optional<vgmtooling::model::node_id>, 2> ym2612_nodes{};
    std::array<std::optional<vgmtooling::model::node_id>, 2> psg_nodes{};
};

struct genesis_trace_append_result {
    vgmtooling::model::node_id source_event_id = 0;
    std::optional<vgmtooling::model::node_id> device_transition_id{};
};

inline genesis_execution_graph_handle begin_genesis_execution_trace(
    vgmtooling::model::musical_execution_graph& graph,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    genesis_execution_graph_handle handle;
    handle.source_trace = begin_vgm_execution_trace(graph, std::move(source), flags);
    return handle;
}

inline vgmtooling::model::node_id ensure_genesis_device_node(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_execution_graph_handle& handle,
    const char* family,
    std::size_t instance) {
    using namespace vgmtooling::model;

    auto& slot = std::string{family} == "YM2612"
        ? handle.ym2612_nodes[instance & 1u]
        : handle.psg_nodes[instance & 1u];
    if (slot.has_value())
        return *slot;

    node device;
    device.kind = node_kind::synthesis_object;
    device.layer = semantic_layer::synthesis;
    device.flow = flow_kind::value;
    device.label = std::string{family} + " instance " + std::to_string(instance & 1u);
    device.attributes.push_back({
        "device_family",
        std::string{family},
        evidence_status::derived,
        1.0,
        "",
    });
    device.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(instance & 1u),
        evidence_status::derived,
        1.0,
        "",
    });
    device.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source_trace.source,
        std::nullopt,
        "device instance identified from VGM command semantics",
        handle.source_trace.provenance_flags,
    });
    slot = graph.add_node(std::move(device));
    return *slot;
}

inline genesis_trace_append_result append_genesis_trace_record(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_execution_graph_handle& handle,
    const command_trace_record& record) {
    using namespace vgmtooling::model;

    genesis_trace_append_result result;
    result.source_event_id = append_vgm_trace_record(graph, handle.source_trace, record);

    const std::uint8_t* payload = record.payload_prefix_size == 0
        ? nullptr
        : record.payload_prefix.data();
    const command_event replay_event{
        record.kind,
        record.tick,
        record.file_offset,
        record.command,
        payload,
        record.payload_prefix_size,
    };

    if (record.kind == command_event_kind::reset) {
        handle.shadow.observe(replay_event);
        return result;
    }

    // Device semantics are only recovered when the bounded record contains the
    // complete command payload. Partial source evidence remains in the source
    // trace and is not promoted into a guessed device transition.
    if (!has_complete_payload(record))
        return result;

    const char* family = nullptr;
    const char* transition_kind = nullptr;
    std::size_t instance = 0;
    std::optional<std::uint8_t> port{};
    std::optional<std::uint8_t> reg{};
    std::optional<std::uint8_t> data{};

    if (record.kind == command_event_kind::ym2612_dac && record.payload_prefix_size >= 1) {
        family = "YM2612";
        transition_kind = "resolved_dac_sample";
        data = record.payload_prefix[0];
    } else if (record.kind == command_event_kind::command) {
        switch (record.command) {
        case 0x52:
        case 0x53:
            if (record.payload_prefix_size >= 2) {
                family = "YM2612";
                transition_kind = "register_write";
                port = static_cast<std::uint8_t>(record.command - 0x52);
                reg = record.payload_prefix[0];
                data = record.payload_prefix[1];
            }
            break;
        case 0xA2:
        case 0xA3:
            if (record.payload_prefix_size >= 2) {
                family = "YM2612";
                transition_kind = "register_write";
                instance = 1;
                port = static_cast<std::uint8_t>(record.command - 0xA2);
                reg = record.payload_prefix[0];
                data = record.payload_prefix[1];
            }
            break;
        case 0x50:
        case 0x30:
            if (record.payload_prefix_size >= 1) {
                family = "SN76489";
                transition_kind = "register_write";
                instance = record.command == 0x30 ? 1 : 0;
                data = record.payload_prefix[0];
            }
            break;
        case 0x4F:
            if (record.payload_prefix_size >= 1) {
                family = "SN76489";
                transition_kind = "stereo_mask";
                data = record.payload_prefix[0];
            }
            break;
        default:
            break;
        }
    }

    // Replay every complete command through the existing source-truth shadow.
    // The graph stores the transition history; this snapshot is a convenient
    // rebuildable view of the state after the observed prefix of execution.
    handle.shadow.observe(replay_event);

    if (family == nullptr || transition_kind == nullptr)
        return result;

    const node_id device_id = ensure_genesis_device_node(graph, handle, family, instance);

    node transition;
    transition.kind = node_kind::trace_event;
    transition.layer = semantic_layer::synthesis;
    transition.flow = flow_kind::event;
    transition.label = std::string{family} + " " + transition_kind;
    transition.active = time_span{{
        time_domain::source,
        static_cast<std::int64_t>(record.tick),
        0,
        0,
    }, std::nullopt};
    transition.attributes.push_back({
        "device_family",
        std::string{family},
        evidence_status::derived,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(instance),
        evidence_status::derived,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "transition_kind",
        std::string{transition_kind},
        evidence_status::derived,
        1.0,
        "",
    });
    if (port.has_value()) {
        transition.attributes.push_back({
            "port",
            static_cast<std::uint64_t>(*port),
            evidence_status::exact,
            1.0,
            "",
        });
    }
    if (reg.has_value()) {
        transition.attributes.push_back({
            "register",
            static_cast<std::uint64_t>(*reg),
            evidence_status::exact,
            1.0,
            "byte",
        });
    }
    if (data.has_value()) {
        transition.attributes.push_back({
            "data",
            static_cast<std::uint64_t>(*data),
            evidence_status::exact,
            1.0,
            "byte",
        });
    }
    transition.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source_trace.source,
        record.kind == command_event_kind::reset
            ? std::optional<std::uint64_t>{}
            : std::optional<std::uint64_t>{record.file_offset},
        "device transition decoded from complete VGM command payload",
        handle.source_trace.provenance_flags,
    });
    result.device_transition_id = graph.add_node(std::move(transition));

    edge caused;
    caused.kind = edge_kind::causes;
    caused.from = result.source_event_id;
    caused.to = *result.device_transition_id;
    caused.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source_trace.source,
        record.file_offset,
        "VGM command deterministically causes this device transition",
        handle.source_trace.provenance_flags,
    });
    graph.add_edge(std::move(caused));

    edge membership;
    membership.kind = edge_kind::contains;
    membership.from = device_id;
    membership.to = *result.device_transition_id;
    membership.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source_trace.source,
        record.file_offset,
        "transition belongs to this decoded device instance",
        handle.source_trace.provenance_flags,
    });
    graph.add_edge(std::move(membership));

    return result;
}

inline genesis_trace_append_result append_genesis_trace_event(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_execution_graph_handle& handle,
    const command_event& event) {
    return append_genesis_trace_record(graph, handle, make_command_trace_record(event));
}

} // namespace gameaudio::vgm
