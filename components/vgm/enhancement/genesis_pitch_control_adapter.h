#pragma once

#include "genesis_performance_adapter.h"

#include <array>
#include <optional>
#include <string>

namespace gameaudio::vgm {

// Source-specific grouping of device-native pitch state changes into a bounded
// performance control attached to one physical synthesis episode. The control
// keeps device-native values and transition provenance. It does not invent MIDI
// pitch bend, continuous interpolation, note identity, or persistent part identity.
struct genesis_pitch_control_graph_handle {
    genesis_performance_graph_handle performance;
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 6>, 2> ym_pitch_parameter{};
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 3>, 2> psg_pitch_parameter{};
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 6>, 2> ym_last_pitch_transition{};
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 3>, 2> psg_last_pitch_transition{};
};

struct genesis_pitch_control_append_result {
    genesis_performance_append_result performance;
    std::optional<vgmtooling::model::node_id> pitch_parameter_id{};
    std::optional<vgmtooling::model::edge_id> pitch_support_edge_id{};
};

inline genesis_pitch_control_graph_handle begin_genesis_pitch_control_trace(
    vgmtooling::model::musical_execution_graph& graph,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    genesis_pitch_control_graph_handle handle;
    handle.performance = begin_genesis_performance_trace(graph, std::move(source), flags);
    return handle;
}

inline vgmtooling::model::node_id add_genesis_pitch_parameter(
    vgmtooling::model::musical_execution_graph& graph,
    const genesis_pitch_control_graph_handle& handle,
    vgmtooling::model::node_id physical_voice_episode_id,
    const command_trace_record& record,
    const char* device_family,
    std::size_t instance,
    std::size_t physical_channel) {
    using namespace vgmtooling::model;

    node parameter;
    parameter.kind = node_kind::parameter;
    parameter.layer = semantic_layer::musical_performance;
    parameter.flow = flow_kind::control;
    parameter.label = std::string{device_family} + " device-native pitch control";
    parameter.active = time_span{{
        time_domain::source,
        static_cast<std::int64_t>(record.tick),
        0,
        0,
    }, std::nullopt};
    parameter.attributes.push_back({
        "parameter_kind",
        std::string{"pitch"},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "representation",
        std::string{"device_native"},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "device_family",
        std::string{device_family},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(instance),
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "physical_channel",
        static_cast<std::uint64_t>(physical_channel),
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "persistent_part_identity",
        std::string{"unresolved"},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.performance.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "device-native pitch control grouped from exact/derived device transitions; interpolation and note semantics remain unresolved",
        handle.performance.execution.source_trace.provenance_flags,
    });
    const node_id parameter_id = graph.add_node(std::move(parameter));

    edge control;
    control.kind = edge_kind::controls;
    control.from = parameter_id;
    control.to = physical_voice_episode_id;
    control.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.performance.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "pitch parameter controls this bounded physical voice episode",
        handle.performance.execution.source_trace.provenance_flags,
    });
    graph.add_edge(std::move(control));

    return parameter_id;
}

inline vgmtooling::model::edge_id add_genesis_pitch_support(
    vgmtooling::model::musical_execution_graph& graph,
    const genesis_pitch_control_graph_handle& handle,
    vgmtooling::model::node_id device_transition_id,
    vgmtooling::model::node_id pitch_parameter_id,
    const char* control_role,
    std::uint16_t device_pitch_code,
    std::optional<std::uint8_t> device_pitch_block) {
    using namespace vgmtooling::model;

    const node* transition = graph.find_node(device_transition_id);
    if (transition == nullptr)
        throw std::invalid_argument("pitch support references an unknown device transition");

    edge support;
    support.kind = edge_kind::contributes_to;
    support.from = device_transition_id;
    support.to = pitch_parameter_id;
    support.active = transition->active;
    support.attributes.push_back({
        "control_role",
        std::string{control_role},
        evidence_status::derived,
        1.0,
        "",
    });
    support.attributes.push_back({
        "device_pitch_code",
        static_cast<std::uint64_t>(device_pitch_code),
        evidence_status::derived,
        1.0,
        "",
    });
    if (device_pitch_block.has_value()) {
        support.attributes.push_back({
            "device_pitch_block",
            static_cast<std::uint64_t>(*device_pitch_block),
            evidence_status::derived,
            1.0,
            "",
        });
    }
    support.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.performance.execution.source_trace.source,
        transition->provenance.empty() ? std::optional<std::uint64_t>{} : transition->provenance[0].byte_offset,
        "device transition establishes one device-native pitch state for this performance control",
        handle.performance.execution.source_trace.provenance_flags,
    });
    return graph.add_edge(std::move(support));
}

inline void close_genesis_pitch_parameter(
    vgmtooling::model::musical_execution_graph& graph,
    std::optional<vgmtooling::model::node_id>& parameter,
    const vgmtooling::model::node* physical_episode) {
    using namespace vgmtooling::model;

    if (!parameter.has_value())
        return;
    node* value = graph.find_node(*parameter);
    if (value != nullptr && physical_episode != nullptr) {
        if (physical_episode->active.has_value() && value->active.has_value())
            value->active->end = physical_episode->active->end;
        if (physical_episode->attributes.size() > 5 &&
            physical_episode->attributes[5].key == "termination_reason") {
            value->attributes.push_back(physical_episode->attributes[5]);
        }
    }
    parameter.reset();
}

inline void sync_closed_genesis_pitch_parameters(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_pitch_control_graph_handle& handle,
    const std::array<std::array<std::optional<vgmtooling::model::node_id>, 6>, 2>& before_ym_episode,
    const std::array<std::array<std::optional<vgmtooling::model::node_id>, 3>, 2>& before_psg_episode) {
    for (std::size_t instance = 0; instance < 2; ++instance) {
        for (std::size_t channel = 0; channel < 6; ++channel) {
            if (before_ym_episode[instance][channel].has_value() &&
                !handle.performance.ym_physical_voice_episode[instance][channel].has_value()) {
                const auto* episode = graph.find_node(*before_ym_episode[instance][channel]);
                close_genesis_pitch_parameter(
                    graph,
                    handle.ym_pitch_parameter[instance][channel],
                    episode);
            }
        }
        for (std::size_t channel = 0; channel < 3; ++channel) {
            if (before_psg_episode[instance][channel].has_value() &&
                !handle.performance.psg_physical_voice_episode[instance][channel].has_value()) {
                const auto* episode = graph.find_node(*before_psg_episode[instance][channel]);
                close_genesis_pitch_parameter(
                    graph,
                    handle.psg_pitch_parameter[instance][channel],
                    episode);
            }
        }
    }
}

inline genesis_pitch_control_append_result append_genesis_pitch_control_record(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_pitch_control_graph_handle& handle,
    const command_trace_record& record) {
    genesis_pitch_control_append_result result;

    const bool before_valid = handle.performance.execution.shadow_continuation_valid;
    std::array<ym2612_state, 2> before_ym{};
    std::array<sn76489_state, 2> before_psg{};
    if (before_valid) {
        for (std::size_t i = 0; i < 2; ++i) {
            before_ym[i] = handle.performance.execution.shadow.ym2612(i);
            before_psg[i] = handle.performance.execution.shadow.psg(i);
        }
    }

    const auto before_ym_episode = handle.performance.ym_physical_voice_episode;
    const auto before_psg_episode = handle.performance.psg_physical_voice_episode;

    result.performance = append_genesis_performance_record(graph, handle.performance, record);

    sync_closed_genesis_pitch_parameters(
        graph,
        handle,
        before_ym_episode,
        before_psg_episode);

    if (record.kind == command_event_kind::reset) {
        for (auto& instance : handle.ym_last_pitch_transition)
            instance.fill(std::nullopt);
        for (auto& instance : handle.psg_last_pitch_transition)
            instance.fill(std::nullopt);
        return result;
    }

    if (!before_valid || !handle.performance.execution.shadow_continuation_valid ||
        !result.performance.execution.device_transition_id.has_value()) {
        return result;
    }

    const vgmtooling::model::node_id device_transition_id = *result.performance.execution.device_transition_id;

    // Record which decoded transition most recently changed each device-native
    // pitch state. This remains a transition history, not a continuous curve.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after_ym = handle.performance.execution.shadow.ym2612(instance);
        const auto& after_psg = handle.performance.execution.shadow.psg(instance);
        for (std::size_t channel = 0; channel < 6; ++channel) {
            if (before_ym[instance].channels[channel].fnum != after_ym.channels[channel].fnum ||
                before_ym[instance].channels[channel].block != after_ym.channels[channel].block) {
                handle.ym_last_pitch_transition[instance][channel] = device_transition_id;
            }
        }
        for (std::size_t channel = 0; channel < 3; ++channel) {
            if (before_psg[instance].channels[channel].tone_period != after_psg.channels[channel].tone_period) {
                handle.psg_last_pitch_transition[instance][channel] = device_transition_id;
            }
        }
    }

    // A newly opened physical episode gets one performance-layer pitch parameter.
    // Its initial state is linked back to the most recent device transition that
    // established the current pitch, even when that transition occurred before
    // the gate/attenuation boundary that opened the episode.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after_ym = handle.performance.execution.shadow.ym2612(instance);
        for (std::size_t channel = 0; channel < 6; ++channel) {
            const auto& before_episode = before_ym_episode[instance][channel];
            const auto& after_episode = handle.performance.ym_physical_voice_episode[instance][channel];
            auto& parameter = handle.ym_pitch_parameter[instance][channel];
            if (!before_episode.has_value() && after_episode.has_value() && !parameter.has_value()) {
                parameter = add_genesis_pitch_parameter(
                    graph,
                    handle,
                    *after_episode,
                    record,
                    "YM2612",
                    instance,
                    channel);
                result.pitch_parameter_id = parameter;
                const auto& support = handle.ym_last_pitch_transition[instance][channel];
                if (support.has_value()) {
                    result.pitch_support_edge_id = add_genesis_pitch_support(
                        graph,
                        handle,
                        *support,
                        *parameter,
                        "initial_state",
                        after_ym.channels[channel].fnum,
                        after_ym.channels[channel].block);
                }
            } else if (after_episode.has_value() && parameter.has_value() &&
                       handle.ym_last_pitch_transition[instance][channel].has_value() &&
                       *handle.ym_last_pitch_transition[instance][channel] == device_transition_id &&
                       (before_ym[instance].channels[channel].fnum != after_ym.channels[channel].fnum ||
                        before_ym[instance].channels[channel].block != after_ym.channels[channel].block)) {
                result.pitch_parameter_id = parameter;
                result.pitch_support_edge_id = add_genesis_pitch_support(
                    graph,
                    handle,
                    device_transition_id,
                    *parameter,
                    "state_change",
                    after_ym.channels[channel].fnum,
                    after_ym.channels[channel].block);
            }
        }

        const auto& after_psg = handle.performance.execution.shadow.psg(instance);
        for (std::size_t channel = 0; channel < 3; ++channel) {
            const auto& before_episode = before_psg_episode[instance][channel];
            const auto& after_episode = handle.performance.psg_physical_voice_episode[instance][channel];
            auto& parameter = handle.psg_pitch_parameter[instance][channel];
            if (!before_episode.has_value() && after_episode.has_value() && !parameter.has_value()) {
                parameter = add_genesis_pitch_parameter(
                    graph,
                    handle,
                    *after_episode,
                    record,
                    "SN76489",
                    instance,
                    channel);
                result.pitch_parameter_id = parameter;
                const auto& support = handle.psg_last_pitch_transition[instance][channel];
                if (support.has_value()) {
                    result.pitch_support_edge_id = add_genesis_pitch_support(
                        graph,
                        handle,
                        *support,
                        *parameter,
                        "initial_state",
                        after_psg.channels[channel].tone_period,
                        std::nullopt);
                }
            } else if (after_episode.has_value() && parameter.has_value() &&
                       handle.psg_last_pitch_transition[instance][channel].has_value() &&
                       *handle.psg_last_pitch_transition[instance][channel] == device_transition_id &&
                       before_psg[instance].channels[channel].tone_period != after_psg.channels[channel].tone_period) {
                result.pitch_parameter_id = parameter;
                result.pitch_support_edge_id = add_genesis_pitch_support(
                    graph,
                    handle,
                    device_transition_id,
                    *parameter,
                    "state_change",
                    after_psg.channels[channel].tone_period,
                    std::nullopt);
            }
        }
    }

    return result;
}

inline genesis_pitch_control_append_result append_genesis_pitch_control_event(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_pitch_control_graph_handle& handle,
    const command_event& event) {
    return append_genesis_pitch_control_record(graph, handle, make_command_trace_record(event));
}

inline void append_genesis_pitch_control_capture(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_pitch_control_graph_handle& handle,
    const command_trace_capture& capture) {
    apply_vgm_capture_quality(graph, handle.performance.execution.source_trace, capture);
    for (std::size_t i = 0; i < capture.count(); ++i)
        append_genesis_pitch_control_record(graph, handle, capture.records()[i]);

    if (capture.overflowed()) {
        const auto before_ym_episode = handle.performance.ym_physical_voice_episode;
        const auto before_psg_episode = handle.performance.psg_physical_voice_episode;
        handle.performance.execution.shadow_continuation_valid = false;
        close_all_genesis_physical_voice_episodes(
            graph,
            handle.performance,
            std::nullopt,
            "capture_overflow");
        sync_closed_genesis_pitch_parameters(
            graph,
            handle,
            before_ym_episode,
            before_psg_episode);
    }
}

} // namespace gameaudio::vgm
