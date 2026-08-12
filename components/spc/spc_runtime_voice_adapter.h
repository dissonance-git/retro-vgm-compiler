#pragma once

#include "spc_snapshot_graph_adapter.h"

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameaudio::spc {

enum class spc_voice_runtime_event_kind : std::uint8_t {
    key_on_accepted = 0,
    sample_phase_started,
    release_entered,
    became_inactive,
    source_latched,
    continuation_lost,
    execution_reset,
};

inline const char* spc_voice_runtime_event_name(spc_voice_runtime_event_kind kind) noexcept {
    switch (kind) {
    case spc_voice_runtime_event_kind::key_on_accepted:
        return "key_on_accepted";
    case spc_voice_runtime_event_kind::sample_phase_started:
        return "sample_phase_started";
    case spc_voice_runtime_event_kind::release_entered:
        return "release_entered";
    case spc_voice_runtime_event_kind::became_inactive:
        return "became_inactive";
    case spc_voice_runtime_event_kind::source_latched:
        return "source_latched";
    case spc_voice_runtime_event_kind::continuation_lost:
        return "continuation_lost";
    case spc_voice_runtime_event_kind::execution_reset:
        return "execution_reset";
    }
    return "unknown";
}

struct spc_voice_runtime_event {
    spc_voice_runtime_event_kind kind = spc_voice_runtime_event_kind::source_latched;
    std::optional<std::uint8_t> voice{};
    std::int64_t tick = 0;
    std::uint64_t tick_rate = 0;
    std::optional<std::uint8_t> source_index{};
    std::optional<std::uint16_t> brr_address{};
    std::optional<std::uint32_t> envelope_value{};
    std::optional<std::uint32_t> pitch_rate{};
    std::optional<std::uint8_t> key_on_delay{};
    std::optional<bool> noise_enabled{};
};

struct spc_runtime_voice_graph_handle {
    spc_snapshot_graph_handle snapshot;
    vgmtooling::model::node_id execution_trace_id = 0;
    std::array<std::optional<vgmtooling::model::node_id>, 8> physical_voice_episode{};
    std::string source;
    vgmtooling::model::provenance_flags provenance_flags =
        vgmtooling::model::to_flags(vgmtooling::model::provenance_flag::none);
};

struct spc_runtime_append_result {
    vgmtooling::model::node_id trace_event_id = 0;
    std::optional<vgmtooling::model::node_id> physical_voice_episode_id{};
};

inline vgmtooling::model::time_coordinate spc_runtime_time(
    std::int64_t tick,
    std::uint64_t tick_rate) noexcept {
    return {vgmtooling::model::time_domain::device, tick, tick_rate, 0};
}

inline spc_runtime_voice_graph_handle begin_spc_runtime_voice_trace(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_snapshot_graph_handle& snapshot,
    std::string source,
    vgmtooling::model::provenance_flags flags) {
    using namespace vgmtooling::model;

    spc_runtime_voice_graph_handle handle;
    handle.snapshot = snapshot;
    handle.source = std::move(source);
    handle.provenance_flags = flags;

    node trace;
    trace.kind = node_kind::execution_trace;
    trace.layer = semantic_layer::synthesis;
    trace.flow = flow_kind::stream;
    trace.label = "S-DSP controlled runtime trace";
    trace.attributes.push_back({
        "runtime_boundary",
        std::string{"instrumented_dsp_internal_state"},
        evidence_status::derived,
        1.0,
        "",
    });
    trace.provenance.push_back({
        evidence_status::exact,
        1.0,
        handle.source,
        std::nullopt,
        "instrumented S-DSP runtime observations",
        flags | provenance_flag::runtime_capture,
    });
    handle.execution_trace_id = graph.add_node(std::move(trace));

    edge continuation;
    continuation.kind = edge_kind::derived_from;
    continuation.from = snapshot.snapshot_id;
    continuation.to = handle.execution_trace_id;
    continuation.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        "controlled execution continues from the exact SPC machine snapshot; hidden DSP runtime microstate is established by the emulator",
        flags | provenance_flag::runtime_capture,
    });
    graph.add_edge(std::move(continuation));

    return handle;
}

inline void close_spc_runtime_voice_episode(
    vgmtooling::model::musical_execution_graph& graph,
    std::optional<vgmtooling::model::node_id>& episode,
    std::int64_t tick,
    std::uint64_t tick_rate,
    const char* reason,
    bool boundary_complete = true) {
    using namespace vgmtooling::model;

    if (!episode.has_value())
        return;

    node* voice = graph.find_node(*episode);
    if (voice != nullptr) {
        if (voice->active.has_value())
            voice->active->end = spc_runtime_time(tick, tick_rate);
        voice->attributes.push_back({
            "termination_reason",
            std::string{reason},
            evidence_status::derived,
            1.0,
            "",
        });
        voice->attributes.push_back({
            "termination_boundary_complete",
            boundary_complete,
            evidence_status::derived,
            1.0,
            "",
        });
    }

    episode.reset();
}

inline void close_all_spc_runtime_voice_episodes(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_voice_graph_handle& handle,
    std::int64_t tick,
    std::uint64_t tick_rate,
    const char* reason,
    bool boundary_complete) {
    for (auto& episode : handle.physical_voice_episode)
        close_spc_runtime_voice_episode(graph, episode, tick, tick_rate, reason, boundary_complete);
}

inline vgmtooling::model::node_id add_spc_runtime_trace_event(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_runtime_voice_graph_handle& handle,
    const spc_voice_runtime_event& runtime_event) {
    using namespace vgmtooling::model;

    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = std::string{"S-DSP "} + spc_voice_runtime_event_name(runtime_event.kind);
    event.active = time_span{spc_runtime_time(runtime_event.tick, runtime_event.tick_rate), std::nullopt};
    event.attributes.push_back({
        "event_kind",
        std::string{spc_voice_runtime_event_name(runtime_event.kind)},
        evidence_status::exact,
        1.0,
        "",
    });
    if (runtime_event.voice.has_value())
        event.attributes.push_back({"physical_voice", static_cast<std::uint64_t>(*runtime_event.voice), evidence_status::exact, 1.0, "slot"});
    if (runtime_event.source_index.has_value())
        event.attributes.push_back({"source_index", static_cast<std::uint64_t>(*runtime_event.source_index), evidence_status::exact, 1.0, "slot"});
    if (runtime_event.brr_address.has_value())
        event.attributes.push_back({"brr_address", static_cast<std::uint64_t>(*runtime_event.brr_address), evidence_status::exact, 1.0, "address"});
    if (runtime_event.envelope_value.has_value())
        event.attributes.push_back({"envelope_value", static_cast<std::uint64_t>(*runtime_event.envelope_value), evidence_status::exact, 1.0, "device_native"});
    if (runtime_event.pitch_rate.has_value())
        event.attributes.push_back({"pitch_rate", static_cast<std::uint64_t>(*runtime_event.pitch_rate), evidence_status::exact, 1.0, "device_native"});
    if (runtime_event.key_on_delay.has_value())
        event.attributes.push_back({"key_on_delay", static_cast<std::uint64_t>(*runtime_event.key_on_delay), evidence_status::exact, 1.0, "device_native"});
    if (runtime_event.noise_enabled.has_value())
        event.attributes.push_back({"noise_enabled", *runtime_event.noise_enabled, evidence_status::exact, 1.0, ""});

    provenance_flags event_flags = handle.provenance_flags | provenance_flag::runtime_capture;
    if (runtime_event.kind == spc_voice_runtime_event_kind::continuation_lost)
        event_flags = event_flags | provenance_flag::incomplete;
    event.provenance.push_back({
        evidence_status::exact,
        1.0,
        handle.source,
        std::nullopt,
        "exact observation from the instrumented DSP runtime boundary",
        event_flags,
    });
    const node_id event_id = graph.add_node(std::move(event));

    edge membership;
    membership.kind = edge_kind::contains;
    membership.from = handle.execution_trace_id;
    membership.to = event_id;
    membership.provenance.push_back({
        evidence_status::exact,
        1.0,
        handle.source,
        std::nullopt,
        "runtime event belongs to this controlled S-DSP execution trace",
        event_flags,
    });
    graph.add_edge(std::move(membership));

    return event_id;
}

inline vgmtooling::model::node_id add_spc_physical_voice_episode(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_runtime_voice_graph_handle& handle,
    const spc_voice_runtime_event& runtime_event,
    vgmtooling::model::node_id trace_event_id) {
    using namespace vgmtooling::model;

    const std::size_t voice_index = *runtime_event.voice;

    node voice;
    voice.kind = node_kind::voice_instance;
    voice.layer = semantic_layer::synthesis;
    voice.flow = flow_kind::stream;
    voice.label = "S-DSP physical voice episode";
    voice.active = time_span{spc_runtime_time(runtime_event.tick, runtime_event.tick_rate), std::nullopt};
    voice.attributes.push_back({"physical_voice", static_cast<std::uint64_t>(voice_index), evidence_status::derived, 1.0, "slot"});
    voice.attributes.push_back({"identity_scope", std::string{"physical_voice_episode"}, evidence_status::derived, 1.0, ""});
    voice.attributes.push_back({"episode_origin", std::string{"observed_key_on_acceptance"}, evidence_status::derived, 1.0, ""});
    voice.attributes.push_back({"persistent_part_identity", std::string{"unresolved"}, evidence_status::derived, 1.0, ""});
    if (runtime_event.source_index.has_value())
        voice.attributes.push_back({"initial_source_index", static_cast<std::uint64_t>(*runtime_event.source_index), evidence_status::derived, 1.0, "slot"});
    if (runtime_event.key_on_delay.has_value())
        voice.attributes.push_back({"initial_key_on_delay", static_cast<std::uint64_t>(*runtime_event.key_on_delay), evidence_status::derived, 1.0, "device_native"});
    voice.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        "physical synthesis episode begins at observed DSP key-on acceptance; this is not a musical note or persistent part",
        handle.provenance_flags | provenance_flag::runtime_capture,
    });
    const node_id episode_id = graph.add_node(std::move(voice));

    edge occupancy;
    occupancy.kind = edge_kind::occupies;
    occupancy.from = episode_id;
    occupancy.to = handle.snapshot.voice_slot_ids[voice_index];
    occupancy.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        "runtime voice episode occupies this S-DSP physical voice slot",
        handle.provenance_flags | provenance_flag::runtime_capture,
    });
    graph.add_edge(std::move(occupancy));

    edge cause;
    cause.kind = edge_kind::causes;
    cause.from = trace_event_id;
    cause.to = episode_id;
    cause.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        "observed key-on acceptance starts this physical voice episode",
        handle.provenance_flags | provenance_flag::runtime_capture,
    });
    graph.add_edge(std::move(cause));

    return episode_id;
}

inline void connect_spc_runtime_event_to_episode(
    vgmtooling::model::musical_execution_graph& graph,
    const spc_runtime_voice_graph_handle& handle,
    vgmtooling::model::node_id trace_event_id,
    vgmtooling::model::node_id episode_id,
    const char* detail) {
    using namespace vgmtooling::model;

    edge relation;
    relation.kind = edge_kind::contributes_to;
    relation.from = trace_event_id;
    relation.to = episode_id;
    relation.provenance.push_back({
        evidence_status::derived,
        1.0,
        handle.source,
        std::nullopt,
        detail,
        handle.provenance_flags | provenance_flag::runtime_capture,
    });
    graph.add_edge(std::move(relation));
}

inline spc_runtime_append_result append_spc_runtime_voice_event(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_voice_graph_handle& handle,
    const spc_voice_runtime_event& runtime_event) {
    using namespace vgmtooling::model;

    const bool global_event =
        runtime_event.kind == spc_voice_runtime_event_kind::continuation_lost ||
        runtime_event.kind == spc_voice_runtime_event_kind::execution_reset;

    if (!global_event) {
        if (!runtime_event.voice.has_value() || *runtime_event.voice >= handle.physical_voice_episode.size())
            throw std::invalid_argument("S-DSP runtime voice event requires a physical voice index in [0, 7]");
    }

    spc_runtime_append_result result;
    result.trace_event_id = add_spc_runtime_trace_event(graph, handle, runtime_event);

    if (runtime_event.kind == spc_voice_runtime_event_kind::continuation_lost) {
        close_all_spc_runtime_voice_episodes(
            graph,
            handle,
            runtime_event.tick,
            runtime_event.tick_rate,
            "semantic_continuation_lost",
            false);
        return result;
    }

    if (runtime_event.kind == spc_voice_runtime_event_kind::execution_reset) {
        close_all_spc_runtime_voice_episodes(
            graph,
            handle,
            runtime_event.tick,
            runtime_event.tick_rate,
            "execution_reset",
            true);
        return result;
    }

    const std::size_t voice_index = *runtime_event.voice;
    auto& episode = handle.physical_voice_episode[voice_index];

    if (runtime_event.kind == spc_voice_runtime_event_kind::key_on_accepted) {
        if (episode.has_value()) {
            close_spc_runtime_voice_episode(
                graph,
                episode,
                runtime_event.tick,
                runtime_event.tick_rate,
                "retriggered_by_key_on",
                true);
        }
        episode = add_spc_physical_voice_episode(graph, handle, runtime_event, result.trace_event_id);
        result.physical_voice_episode_id = episode;
        return result;
    }

    if (!episode.has_value())
        return result;

    result.physical_voice_episode_id = episode;

    switch (runtime_event.kind) {
    case spc_voice_runtime_event_kind::sample_phase_started:
        connect_spc_runtime_event_to_episode(
            graph,
            handle,
            result.trace_event_id,
            *episode,
            "KON delay expired and the physical voice entered sample-producing runtime phase");
        break;
    case spc_voice_runtime_event_kind::release_entered:
        connect_spc_runtime_event_to_episode(
            graph,
            handle,
            result.trace_event_id,
            *episode,
            "physical voice entered release; the episode remains alive until the DSP reports inactivity");
        break;
    case spc_voice_runtime_event_kind::source_latched:
        connect_spc_runtime_event_to_episode(
            graph,
            handle,
            result.trace_event_id,
            *episode,
            "runtime source state belongs to this physical voice episode but does not establish instrument identity");
        break;
    case spc_voice_runtime_event_kind::became_inactive: {
        const node_id episode_id = *episode;
        connect_spc_runtime_event_to_episode(
            graph,
            handle,
            result.trace_event_id,
            episode_id,
            "DSP inactivity terminates this physical voice episode");
        close_spc_runtime_voice_episode(
            graph,
            episode,
            runtime_event.tick,
            runtime_event.tick_rate,
            "became_inactive",
            true);
        result.physical_voice_episode_id = episode_id;
        break;
    }
    case spc_voice_runtime_event_kind::key_on_accepted:
    case spc_voice_runtime_event_kind::continuation_lost:
    case spc_voice_runtime_event_kind::execution_reset:
        break;
    }

    return result;
}

} // namespace gameaudio::spc
