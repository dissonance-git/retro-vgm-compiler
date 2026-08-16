#pragma once

#include "spc_runtime_sample_adapter.h"
#include "spc_runtime_voice_adapter.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace gameaudio::spc {

inline const vgmtooling::model::attribute* find_spc_performance_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::optional<vgmtooling::model::node_id> spc_runtime_event_episode(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id trace_event_id) noexcept {
    using namespace vgmtooling::model;

    for (edge_kind kind : {edge_kind::causes, edge_kind::contributes_to}) {
        const auto relations = graph.edges_from(trace_event_id, kind);
        for (const edge* relation : relations) {
            const node* target = graph.find_node(relation->to);
            if (target != nullptr && target->kind == node_kind::voice_instance)
                return relation->to;
        }
    }
    return std::nullopt;
}

inline std::optional<vgmtooling::model::node_id> spc_runtime_event_sample(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id trace_event_id) noexcept {
    using namespace vgmtooling::model;

    const auto relations = graph.edges_from(trace_event_id, edge_kind::references);
    for (const edge* relation : relations) {
        const node* target = graph.find_node(relation->to);
        if (target != nullptr && target->kind == node_kind::sample_buffer)
            return relation->to;
    }
    return std::nullopt;
}

inline const char* spc_performance_event_kind(const std::string& runtime_kind) noexcept {
    if (runtime_kind == "key_on_accepted")
        return "source_activity_onset";
    if (runtime_kind == "sample_phase_started")
        return "sample_activity_started";
    if (runtime_kind == "release_entered")
        return "source_activity_release";
    if (runtime_kind == "became_inactive")
        return "source_activity_inactive";
    if (runtime_kind == "source_latched")
        return "source_identity_observed";
    return nullptr;
}

inline std::optional<vgmtooling::model::node_id> add_spc_performance_observation(
    vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id trace_event_id) {
    using namespace vgmtooling::model;

    const node* runtime = graph.find_node(trace_event_id);
    if (runtime == nullptr || runtime->kind != node_kind::trace_event ||
        runtime->layer != semantic_layer::synthesis)
        throw std::invalid_argument("SPC performance observation requires an S-DSP runtime trace event");

    const attribute* runtime_kind_item = find_spc_performance_attribute(*runtime, "event_kind");
    const auto* runtime_kind = runtime_kind_item == nullptr
        ? nullptr
        : std::get_if<std::string>(&runtime_kind_item->value);
    if (runtime_kind == nullptr)
        throw std::invalid_argument("SPC runtime trace event has no string event kind");

    const char* performance_kind = spc_performance_event_kind(*runtime_kind);
    if (performance_kind == nullptr)
        return std::nullopt;

    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = std::string{"S-DSP "} + performance_kind;
    event.active = runtime->active;
    event.attributes.push_back({
        "event_kind",
        std::string{performance_kind},
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "source_runtime_event_kind",
        *runtime_kind,
        evidence_status::exact,
        1.0,
        "",
    });

    const auto copy_optional = [&](const char* source_key, const char* target_key) {
        const attribute* item = find_spc_performance_attribute(*runtime, source_key);
        if (item != nullptr) {
            event.attributes.push_back({
                target_key,
                item->value,
                item->status,
                item->confidence,
                item->unit,
            });
        }
    };

    copy_optional("physical_voice", "physical_voice");
    copy_optional("source_index", "source_index");
    copy_optional("brr_address", "brr_address");
    copy_optional("pitch_rate", "device_native_pitch_rate");
    copy_optional("envelope_value", "device_native_envelope_value");
    copy_optional("noise_enabled", "noise_enabled");

    const auto episode_id = spc_runtime_event_episode(graph, trace_event_id);
    if (episode_id.has_value()) {
        event.attributes.push_back({
            "physical_voice_episode_id",
            static_cast<std::uint64_t>(*episode_id),
            evidence_status::derived,
            1.0,
            "node_id",
        });
    }

    const auto sample_id = spc_runtime_event_sample(graph, trace_event_id);
    if (sample_id.has_value()) {
        event.attributes.push_back({
            "runtime_sample_version_id",
            static_cast<std::uint64_t>(*sample_id),
            evidence_status::derived,
            1.0,
            "node_id",
        });
    }

    event.attributes.push_back({
        "persistent_part_identity",
        std::string{"unresolved"},
        evidence_status::derived,
        1.0,
        "",
    });
    event.provenance = runtime->provenance;
    event.provenance.push_back({
        evidence_status::derived,
        1.0,
        runtime->provenance.empty() ? "spc-performance" : runtime->provenance[0].source,
        std::nullopt,
        "musical-performance observation lifted from exact S-DSP runtime state; source activity is not yet a note name, instrument identity, or persistent musical part",
        runtime->provenance.empty()
            ? to_flags(provenance_flag::runtime_capture)
            : runtime->provenance[0].flags | provenance_flag::runtime_capture,
    });

    const node_id event_id = graph.add_node(std::move(event));

    edge derivation;
    derivation.kind = edge_kind::derived_from;
    derivation.from = trace_event_id;
    derivation.to = event_id;
    derivation.provenance = runtime->provenance;
    graph.add_edge(std::move(derivation));

    if (episode_id.has_value()) {
        edge realization;
        realization.kind = edge_kind::realizes;
        realization.from = event_id;
        realization.to = *episode_id;
        realization.provenance = runtime->provenance;
        graph.add_edge(std::move(realization));
    }

    if (sample_id.has_value()) {
        edge reference;
        reference.kind = edge_kind::references;
        reference.from = event_id;
        reference.to = *sample_id;
        reference.attributes.push_back({
            "reference_kind",
            std::string{"event_time_sample_version"},
            evidence_status::derived,
            1.0,
            "",
        });
        reference.provenance = runtime->provenance;
        graph.add_edge(std::move(reference));
    }

    return event_id;
}

} // namespace gameaudio::spc
