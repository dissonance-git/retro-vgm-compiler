#pragma once

#include "vgm_command_trace_capture.h"
#include "../../../model/musical_execution_graph.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameaudio::vgm {

// Handle for an analysis-side materialization of one observed VGM command run.
//
// This adapter intentionally allocates through musical_execution_graph and must
// never be installed directly as a realtime command_observer. Realtime code
// should capture bounded source evidence first and materialize it off the audio
// thread.
struct vgm_execution_trace_handle {
    vgmtooling::model::node_id trace_id = 0;
    std::string source;
    vgmtooling::model::provenance_flags provenance_flags =
        vgmtooling::model::to_flags(vgmtooling::model::provenance_flag::none);
};

inline const char* command_event_kind_name(command_event_kind kind) noexcept {
    switch (kind) {
    case command_event_kind::reset:
        return "reset";
    case command_event_kind::command:
        return "command";
    case command_event_kind::ym2612_dac:
        return "ym2612_dac";
    }
    return "unknown";
}

inline vgm_execution_trace_handle begin_vgm_execution_trace(
    vgmtooling::model::musical_execution_graph& graph,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    using namespace vgmtooling::model;

    node trace;
    trace.kind = node_kind::execution_trace;
    trace.layer = semantic_layer::source_representation;
    trace.flow = flow_kind::stream;
    trace.label = "VGM command execution trace";
    trace.provenance.push_back({
        evidence_status::exact,
        1.0,
        source,
        std::nullopt,
        "command execution materialized outside the realtime observer",
        flags,
    });

    return {graph.add_node(std::move(trace)), std::move(source), flags};
}

inline vgmtooling::model::node_id append_vgm_trace_record(
    vgmtooling::model::musical_execution_graph& graph,
    const vgm_execution_trace_handle& trace,
    const command_trace_record& event) {
    using namespace vgmtooling::model;

    const node* trace_node = graph.find_node(trace.trace_id);
    if (trace_node == nullptr || trace_node->kind != node_kind::execution_trace) {
        throw std::invalid_argument("VGM trace handle does not reference an execution trace");
    }

    node observed;
    observed.kind = node_kind::trace_event;
    observed.layer = semantic_layer::source_representation;
    observed.flow = flow_kind::event;
    observed.label = "VGM command trace event";
    observed.active = time_span{{time_domain::source, static_cast<std::int64_t>(event.tick), 0, 0}, std::nullopt};
    observed.attributes.push_back({
        "event_kind",
        std::string{command_event_kind_name(event.kind)},
        evidence_status::exact,
        1.0,
        "",
    });
    observed.attributes.push_back({
        "command",
        static_cast<std::uint64_t>(event.command),
        evidence_status::exact,
        1.0,
        "byte",
    });
    observed.attributes.push_back({
        "payload_size",
        static_cast<std::uint64_t>(event.payload_size),
        evidence_status::exact,
        1.0,
        "bytes",
    });

    std::optional<std::uint64_t> byte_offset{};
    if (event.kind != command_event_kind::reset)
        byte_offset = event.file_offset;
    observed.provenance.push_back({
        evidence_status::exact,
        1.0,
        trace.source,
        byte_offset,
        "observed by VGM command parser",
        trace.provenance_flags,
    });

    const node_id event_id = graph.add_node(std::move(observed));

    edge membership;
    membership.kind = edge_kind::contains;
    membership.from = trace.trace_id;
    membership.to = event_id;
    membership.provenance.push_back({
        evidence_status::exact,
        1.0,
        trace.source,
        byte_offset,
        "event belongs to this observed execution trace",
        trace.provenance_flags,
    });
    graph.add_edge(std::move(membership));

    return event_id;
}

inline vgmtooling::model::node_id append_vgm_trace_event(
    vgmtooling::model::musical_execution_graph& graph,
    const vgm_execution_trace_handle& trace,
    const command_event& event) {
    return append_vgm_trace_record(graph, trace, make_command_trace_record(event));
}

// Materialize one bounded realtime capture window on a non-realtime thread.
// If the fixed-capacity capture overflowed, the execution trace is explicitly
// marked incomplete and the dropped-event count remains inspectable.
inline vgm_execution_trace_handle materialize_vgm_command_capture(
    vgmtooling::model::musical_execution_graph& graph,
    const command_trace_capture& capture,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    using namespace vgmtooling::model;

    if (capture.overflowed())
        flags = flags | provenance_flag::incomplete;

    auto trace = begin_vgm_execution_trace(graph, std::move(source), flags);
    for (std::size_t i = 0; i < capture.count(); ++i)
        append_vgm_trace_record(graph, trace, capture.records()[i]);

    if (capture.overflowed()) {
        node* trace_node = graph.find_node(trace.trace_id);
        trace_node->attributes.push_back({
            "capture_overflow",
            true,
            evidence_status::exact,
            1.0,
            "",
        });
        trace_node->attributes.push_back({
            "dropped_events",
            capture.dropped(),
            evidence_status::exact,
            1.0,
            "events",
        });
    }

    return trace;
}

} // namespace gameaudio::vgm
