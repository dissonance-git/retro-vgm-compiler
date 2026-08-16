#pragma once

#include "genesis_pitch_control_adapter.h"
#include "ym2612_episode_synthesis_snapshot.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameaudio::vgm {

struct genesis_fm_semantic_graph_handle {
    genesis_pitch_control_graph_handle pitch;
};

struct genesis_fm_semantic_append_result {
    genesis_pitch_control_append_result pitch;
    std::optional<vgmtooling::model::node_id> ym2612_synthesis_snapshot_id{};
};

inline genesis_fm_semantic_graph_handle begin_genesis_fm_semantic_trace(
    vgmtooling::model::musical_execution_graph& graph,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    genesis_fm_semantic_graph_handle handle;
    handle.pitch = begin_genesis_pitch_control_trace(graph, std::move(source), flags);
    return handle;
}

inline const vgmtooling::model::attribute* find_genesis_fm_semantic_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::optional<std::uint64_t> genesis_fm_semantic_uint_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    const auto* item = find_genesis_fm_semantic_attribute(value, key);
    if (item == nullptr)
        return std::nullopt;
    const auto* number = std::get_if<std::uint64_t>(&item->value);
    if (number == nullptr)
        return std::nullopt;
    return *number;
}

inline std::optional<std::string> genesis_fm_semantic_string_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    const auto* item = find_genesis_fm_semantic_attribute(value, key);
    if (item == nullptr)
        return std::nullopt;
    const auto* text = std::get_if<std::string>(&item->value);
    if (text == nullptr)
        return std::nullopt;
    return *text;
}

inline genesis_fm_semantic_append_result append_genesis_fm_semantic_record(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_fm_semantic_graph_handle& handle,
    const command_trace_record& record) {
    using namespace vgmtooling::model;

    genesis_fm_semantic_append_result result;
    result.pitch = append_genesis_pitch_control_record(graph, handle.pitch, record);

    if (!result.pitch.performance.physical_voice_episode_id.has_value() ||
        !result.pitch.performance.performance_event_id.has_value()) {
        return result;
    }

    const node* event = graph.find_node(*result.pitch.performance.performance_event_id);
    const node* episode = graph.find_node(*result.pitch.performance.physical_voice_episode_id);
    if (event == nullptr || episode == nullptr || !event->active.has_value())
        throw std::logic_error("new Genesis performance episode is missing its graph objects");

    const auto family = genesis_fm_semantic_string_attribute(*event, "device_family");
    if (!family.has_value() || *family != "YM2612")
        return result;

    const auto instance = genesis_fm_semantic_uint_attribute(*event, "instance");
    const auto channel = genesis_fm_semantic_uint_attribute(*event, "physical_channel");
    if (!instance.has_value() || !channel.has_value() || *instance > 1 || *channel > 5)
        throw std::logic_error("new YM2612 performance episode has invalid device coordinates");

    const auto& chip = handle.pitch.performance.execution.shadow.ym2612(
        static_cast<std::size_t>(*instance));
    const auto snapshot = capture_ym2612_episode_synthesis_snapshot(
        chip,
        static_cast<std::size_t>(*instance),
        static_cast<std::size_t>(*channel));

    std::optional<std::uint64_t> source_offset;
    if (!event->provenance.empty())
        source_offset = event->provenance.front().byte_offset;

    result.ym2612_synthesis_snapshot_id = add_ym2612_episode_synthesis_snapshot(
        graph,
        *result.pitch.performance.physical_voice_episode_id,
        snapshot,
        event->active->start,
        handle.pitch.performance.execution.source_trace.source,
        source_offset);
    return result;
}

inline genesis_fm_semantic_append_result append_genesis_fm_semantic_event(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_fm_semantic_graph_handle& handle,
    const command_event& event) {
    return append_genesis_fm_semantic_record(
        graph,
        handle,
        make_command_trace_record(event));
}

inline void append_genesis_fm_semantic_capture(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_fm_semantic_graph_handle& handle,
    const command_trace_capture& capture) {
    apply_vgm_capture_quality(
        graph,
        handle.pitch.performance.execution.source_trace,
        capture);
    for (std::size_t index = 0; index < capture.count(); ++index)
        append_genesis_fm_semantic_record(graph, handle, capture.records()[index]);
    if (capture.overflowed())
        handle.pitch.performance.execution.shadow_continuation_valid = false;
}

} // namespace gameaudio::vgm
