#pragma once

#include "genesis_execution_graph_adapter.h"

#include <array>
#include <optional>
#include <string>

namespace gameaudio::vgm {

// Conservative bridge from exact/derived Genesis device state into the musical
// performance layer. It intentionally models only observations that survive a
// strong source-level test. Physical sounding episodes are synthesis identities,
// not MIDI notes, instruments, parts, or persistent musical-source identities.
struct genesis_performance_graph_handle {
    genesis_execution_graph_handle execution;
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 6>, 2> ym_physical_voice_episode{};
    std::array<std::array<std::optional<vgmtooling::model::node_id>, 3>, 2> psg_physical_voice_episode{};
};

struct genesis_performance_append_result {
    genesis_trace_append_result execution;
    std::optional<vgmtooling::model::node_id> performance_event_id{};
    std::optional<vgmtooling::model::node_id> physical_voice_episode_id{};
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

inline vgmtooling::model::node_id add_genesis_physical_voice_episode(
    vgmtooling::model::musical_execution_graph& graph,
    const genesis_performance_graph_handle& handle,
    vgmtooling::model::node_id device_id,
    const command_trace_record& record,
    const char* device_family,
    std::size_t instance,
    std::size_t physical_channel) {
    using namespace vgmtooling::model;

    node voice;
    voice.kind = node_kind::voice_instance;
    voice.layer = semantic_layer::synthesis;
    voice.flow = flow_kind::stream;
    voice.label = std::string{device_family} + " physical voice episode";
    voice.active = time_span{{
        time_domain::source,
        static_cast<std::int64_t>(record.tick),
        0,
        0,
    }, std::nullopt};
    voice.attributes.push_back({
        "device_family",
        std::string{device_family},
        evidence_status::derived,
        1.0,
        "",
    });
    voice.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(instance),
        evidence_status::derived,
        1.0,
        "",
    });
    voice.attributes.push_back({
        "physical_channel",
        static_cast<std::uint64_t>(physical_channel),
        evidence_status::derived,
        1.0,
        "",
    });
    voice.attributes.push_back({
        "identity_scope",
        std::string{"physical_voice_episode"},
        evidence_status::derived,
        1.0,
        "",
    });
    voice.attributes.push_back({
        "persistent_part_identity",
        std::string{"unresolved"},
        evidence_status::derived,
        1.0,
        "",
    });
    voice.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "bounded physical synthesis episode inferred from conservative device activity; not a persistent musical part",
        handle.execution.source_trace.provenance_flags,
    });
    const node_id voice_id = graph.add_node(std::move(voice));

    edge membership;
    membership.kind = edge_kind::contains;
    membership.from = device_id;
    membership.to = voice_id;
    membership.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.execution.source_trace.source,
        std::optional<std::uint64_t>{record.file_offset},
        "physical voice episode belongs to this synthesis device instance",
        handle.execution.source_trace.provenance_flags,
    });
    graph.add_edge(std::move(membership));

    return voice_id;
}

inline void close_genesis_physical_voice_episode(
    vgmtooling::model::musical_execution_graph& graph,
    std::optional<vgmtooling::model::node_id>& episode,
    std::optional<std::int64_t> end_tick,
    const char* reason) {
    using namespace vgmtooling::model;

    if (!episode.has_value())
        return;
    node* voice = graph.find_node(*episode);
    if (voice != nullptr) {
        if (end_tick.has_value() && voice->active.has_value()) {
            voice->active->end = time_coordinate{
                time_domain::source,
                *end_tick,
                0,
                0,
            };
        }
        voice->attributes.push_back({
            "termination_reason",
            std::string{reason},
            evidence_status::derived,
            1.0,
            "",
        });
    }
    episode.reset();
}

inline void close_all_genesis_physical_voice_episodes(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_performance_graph_handle& handle,
    std::optional<std::int64_t> end_tick,
    const char* reason) {
    for (auto& instance : handle.ym_physical_voice_episode) {
        for (auto& episode : instance)
            close_genesis_physical_voice_episode(graph, episode, end_tick, reason);
    }
    for (auto& instance : handle.psg_physical_voice_episode) {
        for (auto& episode : instance)
            close_genesis_physical_voice_episode(graph, episode, end_tick, reason);
    }
}

inline vgmtooling::model::node_id add_genesis_performance_event(
    vgmtooling::model::musical_execution_graph& graph,
    const genesis_performance_graph_handle& handle,
    vgmtooling::model::node_id device_transition_id,
    std::optional<vgmtooling::model::node_id> physical_voice_episode_id,
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

    if (physical_voice_episode_id.has_value()) {
        edge realization;
        realization.kind = edge_kind::realizes;
        realization.from = event_id;
        realization.to = *physical_voice_episode_id;
        realization.provenance.push_back({
            evidence_status::derived,
            1.0,
            handle.execution.source_trace.source,
            std::optional<std::uint64_t>{record.file_offset},
            "performance event is realized through this bounded physical voice episode",
            handle.execution.source_trace.provenance_flags,
        });
        graph.add_edge(std::move(realization));
    }

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

    if (record.kind == command_event_kind::reset) {
        close_all_genesis_physical_voice_episodes(
            graph,
            handle,
            static_cast<std::int64_t>(record.tick),
            "execution_reset");
        return result;
    }

    if (!before_valid || !handle.execution.shadow_continuation_valid) {
        // A lost/unreplayable interval invalidates any physical-episode continuity
        // that depended on observing every state transition.
        close_all_genesis_physical_voice_episodes(
            graph,
            handle,
            static_cast<std::int64_t>(record.tick),
            "semantic_continuation_lost");
        return result;
    }

    if (!result.execution.device_transition_id.has_value())
        return result;

    // If state leaves the subset where a simple channel-level pitched
    // interpretation is valid, close the bounded episode as an interpretation
    // boundary rather than manufacturing a musical release.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after_ym = handle.execution.shadow.ym2612(instance);
        for (std::size_t channel = 0; channel < after_ym.channels.size(); ++channel) {
            auto& episode = handle.ym_physical_voice_episode[instance][channel];
            if (episode.has_value() && !ym_simple_pitched_channel(after_ym, channel)) {
                close_genesis_physical_voice_episode(
                    graph,
                    episode,
                    static_cast<std::int64_t>(record.tick),
                    "simple_pitched_interpretation_lost");
            }
        }

        const auto& after_psg = handle.execution.shadow.psg(instance);
        for (std::size_t channel = 0; channel < 3; ++channel) {
            auto& episode = handle.psg_physical_voice_episode[instance][channel];
            if (episode.has_value() &&
                (after_psg.channels[channel].tone_period == 0 ||
                 !psg_tone_channel_routed(after_psg, channel))) {
                close_genesis_physical_voice_episode(
                    graph,
                    episode,
                    static_cast<std::int64_t>(record.tick),
                    "simple_pitched_interpretation_lost");
            }
        }
    }

    // YM2612: only the strongest ordinary channel-level case is promoted.
    // Full four-operator 0 -> F gate starts a physical voice episode and a
    // pitched-activity onset. A later all-operators-off boundary closes the same
    // episode even if partial operator re-keying occurred in between.
    for (std::size_t instance = 0; instance < 2; ++instance) {
        const auto& after = handle.execution.shadow.ym2612(instance);
        const auto& before = before_ym[instance];
        for (std::size_t channel = 0; channel < before.channels.size(); ++channel) {
            const std::uint8_t old_mask = before.channels[channel].operator_key_mask;
            const std::uint8_t new_mask = after.channels[channel].operator_key_mask;
            auto& episode = handle.ym_physical_voice_episode[instance][channel];

            if (!episode.has_value() && old_mask == 0 && new_mask == 0x0F &&
                ym_simple_pitched_channel(after, channel)) {
                const auto device_id = handle.execution.ym2612_nodes[instance];
                if (!device_id.has_value())
                    return result;
                episode = add_genesis_physical_voice_episode(
                    graph,
                    handle,
                    *device_id,
                    record,
                    "YM2612",
                    instance,
                    channel);
                result.physical_voice_episode_id = episode;
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    episode,
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
            if (episode.has_value() && old_mask != 0 && new_mask == 0 &&
                ym_simple_pitched_channel(before, channel)) {
                result.physical_voice_episode_id = episode;
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    episode,
                    record,
                    "pitched_activity_release",
                    "YM2612",
                    instance,
                    channel,
                    before.channels[channel].fnum,
                    before.channels[channel].block,
                    old_mask);
                close_genesis_physical_voice_episode(
                    graph,
                    episode,
                    static_cast<std::int64_t>(record.tick),
                    "pitched_activity_release");
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
            auto& episode = handle.psg_physical_voice_episode[instance][channel];

            if (!episode.has_value() && was_off && !is_off && after.channels[channel].tone_period != 0 &&
                psg_tone_channel_routed(after, channel)) {
                const auto device_id = handle.execution.psg_nodes[instance];
                if (!device_id.has_value())
                    return result;
                episode = add_genesis_physical_voice_episode(
                    graph,
                    handle,
                    *device_id,
                    record,
                    "SN76489",
                    instance,
                    channel);
                result.physical_voice_episode_id = episode;
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    episode,
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
            if (episode.has_value() && !was_off && is_off && before.channels[channel].tone_period != 0 &&
                psg_tone_channel_routed(before, channel)) {
                result.physical_voice_episode_id = episode;
                result.performance_event_id = add_genesis_performance_event(
                    graph,
                    handle,
                    *result.execution.device_transition_id,
                    episode,
                    record,
                    "pitched_activity_release",
                    "SN76489",
                    instance,
                    channel,
                    before.channels[channel].tone_period,
                    std::nullopt,
                    before.channels[channel].attenuation);
                close_genesis_physical_voice_episode(
                    graph,
                    episode,
                    static_cast<std::int64_t>(record.tick),
                    "pitched_activity_release");
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
    if (capture.overflowed()) {
        handle.execution.shadow_continuation_valid = false;
        close_all_genesis_physical_voice_episodes(
            graph,
            handle,
            std::nullopt,
            "capture_overflow");
    }
}

} // namespace gameaudio::vgm
