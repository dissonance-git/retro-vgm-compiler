#pragma once

#include "spc_runtime_capture.h"
#include "spc_runtime_sample_adapter.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace gameaudio::spc {

struct spc_runtime_capture_window_view {
    const spc_runtime_capture_record* records = nullptr;
    std::size_t count = 0;
    bool overflowed = false;
    std::uint64_t dropped = 0;
    const spc_runtime_capture_record* first_dropped = nullptr;
    std::uint64_t next_trace_index = 0;
};

inline spc_runtime_capture_window_view view_spc_runtime_capture(
    const spc_runtime_capture& capture) noexcept {
    return {
        capture.records(),
        capture.count(),
        capture.overflowed(),
        capture.dropped(),
        capture.first_dropped_record(),
        capture.next_trace_index(),
    };
}

struct spc_runtime_capture_materializer_state {
    std::optional<std::uint64_t> expected_trace_index{};
};

struct spc_runtime_capture_materialization_options {
    spc_runtime_sample_graph_handle* sample_graph = nullptr;
    const spc_ram_shadow* ram_shadow = nullptr;
};

struct spc_runtime_capture_materialization_result {
    std::size_t records_materialized = 0;
    std::size_t continuity_breaks = 0;
    std::size_t samples_materialized = 0;
    std::size_t samples_reused = 0;
    std::size_t samples_deferred = 0;
    std::optional<vgmtooling::model::node_id> last_trace_event_id{};
};

inline bool spc_capture_record_can_observe_sample(
    const spc_runtime_capture_record& record) noexcept {
    const bool source_phase =
        record.kind == spc_voice_runtime_event_kind::sample_phase_started ||
        record.kind == spc_voice_runtime_event_kind::source_latched;
    return source_phase &&
        has_field(record.fields, spc_runtime_capture_field::source_index) &&
        has_field(record.fields, spc_runtime_capture_field::brr_address);
}

inline void annotate_spc_capture_trace_record(
    vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id trace_event_id,
    const spc_runtime_capture_record& record) {
    using namespace vgmtooling::model;

    node* event = graph.find_node(trace_event_id);
    if (event == nullptr)
        throw std::invalid_argument("SPC capture materializer cannot annotate an unknown trace event");

    event->attributes.push_back({
        "trace_index",
        record.trace_index,
        evidence_status::exact,
        1.0,
        "observation_order",
    });
    event->attributes.push_back({
        "ram_write_serial",
        record.ram_write_serial,
        evidence_status::exact,
        1.0,
        "write_sequence",
    });
    if (has_field(record.fields, spc_runtime_capture_field::directory_loop_address)) {
        event->attributes.push_back({
            "directory_loop_address",
            static_cast<std::uint64_t>(record.directory_loop_address),
            evidence_status::exact,
            1.0,
            "address",
        });
    }
}

inline vgmtooling::model::node_id append_spc_capture_continuity_break(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_voice_graph_handle& runtime,
    std::uint64_t trace_index,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint64_t ram_write_serial,
    const char* reason,
    std::uint64_t dropped) {
    using namespace vgmtooling::model;

    spc_voice_runtime_event gap;
    gap.kind = spc_voice_runtime_event_kind::continuation_lost;
    gap.tick = tick;
    gap.tick_rate = tick_rate;
    const auto appended = append_spc_runtime_voice_event(graph, runtime, gap);

    node* event = graph.find_node(appended.trace_event_id);
    if (event != nullptr) {
        event->attributes.push_back({"trace_index", trace_index, evidence_status::exact, 1.0, "observation_order"});
        event->attributes.push_back({"ram_write_serial", ram_write_serial, evidence_status::exact, 1.0, "write_sequence"});
        event->attributes.push_back({"continuity_break_reason", std::string{reason}, evidence_status::derived, 1.0, ""});
        event->attributes.push_back({"dropped_records", dropped, evidence_status::exact, 1.0, "observations"});
    }

    return appended.trace_event_id;
}

inline spc_runtime_sample_observation make_spc_runtime_sample_observation(
    const spc_runtime_capture_record& record,
    const spc_runtime_append_result& runtime_result) {
    spc_runtime_sample_observation observation;
    observation.trace_event_id = runtime_result.trace_event_id;
    observation.physical_voice_episode_id = runtime_result.physical_voice_episode_id;
    observation.source_index = record.source_index;
    observation.start_address = record.brr_address;
    if (has_field(record.fields, spc_runtime_capture_field::directory_loop_address))
        observation.directory_loop_address = record.directory_loop_address;
    observation.ram_write_serial = record.ram_write_serial;
    observation.tick = record.tick;
    observation.tick_rate = record.tick_rate;
    return observation;
}

inline spc_runtime_capture_materialization_result materialize_spc_runtime_capture_window(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_voice_graph_handle& runtime,
    spc_runtime_capture_materializer_state& state,
    const spc_runtime_capture_window_view& window,
    spc_runtime_capture_materialization_options options = {}) {
    using namespace vgmtooling::model;

    if ((options.sample_graph == nullptr) != (options.ram_shadow == nullptr))
        throw std::invalid_argument("SPC runtime sample materialization requires both sample graph and RAM shadow");
    if (window.count != 0 && window.records == nullptr)
        throw std::invalid_argument("SPC runtime capture window has records but no storage pointer");
    if (window.overflowed && window.first_dropped == nullptr)
        throw std::invalid_argument("overflowed SPC runtime capture window lacks first dropped boundary record");

    spc_runtime_capture_materialization_result result;

    for (std::size_t i = 0; i < window.count; ++i) {
        const auto& record = window.records[i];

        if (state.expected_trace_index.has_value() &&
            record.trace_index != *state.expected_trace_index) {
            result.last_trace_event_id = append_spc_capture_continuity_break(
                graph,
                runtime,
                *state.expected_trace_index,
                record.tick,
                record.tick_rate,
                record.ram_write_serial,
                "trace_index_discontinuity",
                record.trace_index > *state.expected_trace_index
                    ? record.trace_index - *state.expected_trace_index
                    : 0);
            ++result.continuity_breaks;
        }

        const auto runtime_event = make_spc_voice_runtime_event(record);
        const auto appended = append_spc_runtime_voice_event(graph, runtime, runtime_event);
        annotate_spc_capture_trace_record(graph, appended.trace_event_id, record);
        result.last_trace_event_id = appended.trace_event_id;
        ++result.records_materialized;

        if (options.sample_graph != nullptr &&
            spc_capture_record_can_observe_sample(record)) {
            const auto sample_result = materialize_spc_runtime_sample(
                graph,
                *options.sample_graph,
                *options.ram_shadow,
                make_spc_runtime_sample_observation(record, appended));
            switch (sample_result.status) {
            case spc_runtime_sample_status::materialized:
                ++result.samples_materialized;
                break;
            case spc_runtime_sample_status::reused:
                ++result.samples_reused;
                break;
            case spc_runtime_sample_status::shadow_not_synchronized:
            case spc_runtime_sample_status::ram_changed_after_observation:
                ++result.samples_deferred;
                break;
            }
        }

        state.expected_trace_index = record.trace_index + 1u;
    }

    if (window.overflowed) {
        const auto& dropped = *window.first_dropped;
        result.last_trace_event_id = append_spc_capture_continuity_break(
            graph,
            runtime,
            dropped.trace_index,
            dropped.tick,
            dropped.tick_rate,
            dropped.ram_write_serial,
            "capture_overflow",
            window.dropped);
        ++result.continuity_breaks;
        state.expected_trace_index = window.next_trace_index;
    } else if (window.count == 0 && !state.expected_trace_index.has_value()) {
        state.expected_trace_index = window.next_trace_index;
    }

    return result;
}

} // namespace gameaudio::spc
