#pragma once

#include "genesis_execution_graph_adapter.h"

#include <array>
#include <optional>
#include <string>

namespace gameaudio::vgm {

// Conservative bridge from exact/derived Genesis device state into the musical
// performance layer. It intentionally models only observations that survive a
// strong source-level test. It does not assign MIDI notes, instruments, parts,
// or persistent musical-source identity.
struct genesis_performance_graph_handle {
    genesis_execution_graph_handle execution;
};

struct genesis_performance_append_result {
    genesis_trace_append_result execution;
    std::optional<vgmtooling::model::node_id> performance_event_id{};
};

inline genesis_performance_graph_handle begin_genesis_performance_trace(
    vgmtooling::model::musical_execution_graph& graph,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    genesis_performance_graph_handle handle;
    handle.execution = begin_genesis_execution_trace(graph, std::move(source), flags);
    return handle;
}

inline bool ym_simple_pitched_channel(const ym2612_state& state, std::size_t channel) noexcept {
    if (channel >= state.channels.size())
        return false;
    if (state.channels[channel].fnum == 0)
        return false;

    // Channel 3 special/CSM modes can expose independent operator pitches and
    // timer-driven keying. One channel-level note object would be a lossy guess.
    if (channel == 2 && state.channel3_mode != 0)
        return false;

    // With DAC enabled, channel 6 no longer has ordinary FM-channel acoustic
    // semantics, even if key-gate registers are still written.
    if (channel == 5 && state.dac_enabled)
        return false;

    return true;
}

inline bool psg_tone_channel_routed(const sn76489_state& state, std::size_t channel) noexcept {
    if (channel >= 3)
        return false;
    const std::uint8_t right = static_cast<std::uint8_t>(1u << channel);
    const std::uint8_t left = static_cast<std::uint8_t>(1u << (channel + 4));
    return (state.stereo_mask & static_cast<std::uint8_t>(right | left)) != 0;
}

inline vgmtooling::model::node_id add_genesis_performance_event(
    vgmtooling::model::musical_execution_graph& graph,
    const genesis_performance_graph_handle& handle,
    vgmtooling::model::node_id device_transition_id,
    const command_trace_record& record,
    const char* event_kind,
    const char* device_family,
    std::size_t instance,
    std::size_t physical_channel,
    const std::optional<std::uint16_t>& pitch_code,
    const std::optional<std::uint8_t>& pitch_block,
    const std::optional<std::uint8_t>& gate_or_level) {
    using namespace vgmtooling::model;

    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = std::string{device_family} + " " + event_kind;
    event.active = time_span{{
        time_domain::source,
        static_cast<std::int64_t>(record.tick),
        0,
        0,
    }, std::nullopt};
    event.attributes.push_back({
        "event_kind",
        std::string{event_kind},
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "device_family",
        std::string{device_family},
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(instance),
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "physical_channel",
        static_cast<std::uint64_t>(physical_channel),
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "persistent_part_identity",
        std::string{"unresolved"},
        evidence_status::derived,
        1.0,
        "",
    });
    if (pitch_code.has_value()) {
        event.attributes.push_back({
            "device_pitch_code",
            static_cast<std::uint64_t>(*pitch_code),
            evidence_status::derived,
            1.0,
            "",
        });
    }
    if (pitch_block.has_value()) {
        event.attributes.push_back({
            "device_pitch_block",
            static_cast<std::uint64_t>(*pitch_block),
            evidence_status::derived,
            1.0,
            "",
        });
    }
    if (gate_or_level.has_value()) {
        event.attributes.push_back({
            "gate_or_level",
            static_cast<std::uint64_t>(*gate_or_level),
            evidence_status::derived,
            1.0,
            "",
        });
    }
    event.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "conservative pitched-activity observation derived from a decoded device-state transition; not authored note or persistent part identity",
        handle.execution.source_trace.provenance_flags,
    });
    const node_id event_id = graph.add_node(std::move(event));

    edge derivation;
    derivation.kind = edge_kind::derived_from;
    derivation.from = device_transition_id;
    derivation.to = event_id;
    derivation.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "performance observation retains its device-transition support",
        handle.execution.source_trace.provenance_flags,
    });
    graph.add_edge(std::move(derivation));

    return event_id;
}

inline genesis_performance_append_result append_genesis_performance_record(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_performance_graph_handle& handle,
    const command_trace_record& record) {
    genesis_performance_append_result result;

    const bool before_valid = handle.execution.shadow_continuation_valid;
    std::array<ym2612_state, 2> before_ym{};
    std::array<sn76489_state, 2> before_psg{};
    if (before_valid) {
        for (std::size_t i = 0; i < 2; ++i) {
            before_ym[i] = handle.execution.shadow.ym2612(i);
            before_psg[i] = handle.execution.shadow.psg(i);
        }
    }

    result.execution = append_genesis_trace_record(graph, handle.execution, record);

    if (!before_valid || !handle.execution.shadow_continuation_valid ||
        !result.execution.device_transition_id.has_value()) {
        return result;
    }

    // YM2612: only the strongest ordinary channel-level case is promoted.
    // Full four-operator 0 -> F gate is a pitched-activity onset; F -> 0 is a
    // release. Partial operator re-keying remains device truth because whether
    // it constitutes a new musical note depends on algorithm/carrier context.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after = handle.execution.shadow.ym2612(instance);
        const auto& before = before_ym[instance];
        for (std::size_t channel = 0; channel < before.channels.size(); ++channel) {
            const std::uint8_t old_mask = before.channels[channel].operator_key_mask;
            const std::uint8_t new_mask = after.channels[channel].operator_key_mask;
            if (old_mask == 0 && new_mask == 0x0F && ym_simple_pitched_channel(after, channel)) {
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    record,
                    "pitched_activity_onset",
                    "YM2612",
                    instance,
                    channel,
                    after.channels[channel].fnum,
                    after.channels[channel].block,
                    new_mask);
                return result;
            }
            if (old_mask == 0x0F && new_mask == 0 && ym_simple_pitched_channel(before, channel)) {
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    record,
                    "pitched_activity_release",
                    "YM2612",
                    instance,
                    channel,
                    before.channels[channel].fnum,
                    before.channels[channel].block,
                    old_mask);
                return result;
            }
        }
    }

    // SN76489: for the three tone channels, attenuation crossing the hardware
    // mute value is a strong activity boundary when a nonzero tone period and
    // at least one stereo route exist. Noise is deliberately left unresolved.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after = handle.execution.shadow.psg(instance);
        const auto& before = before_psg[instance];
        for (std::size_t channel = 0; channel < 3; ++channel) {
            const bool was_off = before.channels[channel].attenuation == 0x0F;
            const bool is_off = after.channels[channel].attenuation == 0x0F;
            if (was_off && !is_off && after.channels[channel].tone_period != 0 &&
                psg_tone_channel_routed(after, channel)) {
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    record,
                    "pitched_activity_onset",
                    "SN76489",
                    instance,
                    channel,
                    after.channels[channel].tone_period,
                    std::nullopt,
                    after.channels[channel].attenuation);
                return result;
            }
            if (!was_off && is_off && before.channels[channel].tone_period != 0 &&
                psg_tone_channel_routed(before, channel)) {
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    record,
                    "pitched_activity_release",
                    "SN76489",
                    instance,
                    channel,
                    before.channels[channel].tone_period,
                    std::nullopt,
                    before.channels[channel].attenuation);
                return result;
            }
        }
    }

    return result;
}

inline genesis_performance_append_result append_genesis_performance_event(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_performance_graph_handle& handle,
    const command_event& event) {
    return append_genesis_performance_record(graph, handle, make_command_trace_record(event));
}

inline void append_genesis_performance_capture(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_performance_graph_handle& handle,
    const command_trace_capture& capture) {
    apply_vgm_capture_quality(graph, handle.execution.source_trace, capture);
    for (std::size_t i = 0; i < capture.count(); ++i)
        append_genesis_performance_record(graph, handle, capture.records()[i]);
    if (capture.overflowed())
        handle.execution.shadow_continuation_valid = false;
}

} // namespace gameaudio::vgm
