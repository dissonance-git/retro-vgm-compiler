#pragma once

#include "spc_runtime_voice_adapter.h"
#include "../../model/analysis_feature.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace gameaudio::spc {

inline const vgmtooling::model::attribute* find_spc_analysis_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline vgmtooling::model::analysis_feature spc_feature_from_attribute(
    const vgmtooling::model::node& event,
    vgmtooling::model::node_id event_id,
    const char* attribute_key,
    const char* feature_name) {
    using namespace vgmtooling::model;

    const attribute* item = find_spc_analysis_attribute(event, attribute_key);
    if (item == nullptr)
        throw std::invalid_argument("required SPC analysis attribute is missing");

    analysis_feature feature = present_feature(
        feature_name,
        item->value,
        item->status,
        item->confidence,
        item->unit);
    feature.support_nodes.push_back(event_id);
    feature.provenance = event.provenance;
    return feature;
}

inline vgmtooling::model::analysis_feature_set extract_spc_runtime_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id) {
    using namespace vgmtooling::model;

    const node* event = graph.find_node(event_id);
    if (event == nullptr)
        throw std::invalid_argument("SPC analysis feature extraction references an unknown node");
    if (event->kind != node_kind::trace_event || event->layer != semantic_layer::synthesis)
        throw std::invalid_argument("SPC analysis features require an S-DSP runtime trace event");

    analysis_feature_set features;
    features.add(spc_feature_from_attribute(*event, event_id, "event_kind", "event_kind"));

    const attribute* kind_attribute = find_spc_analysis_attribute(*event, "event_kind");
    const auto* event_kind = kind_attribute == nullptr
        ? nullptr
        : std::get_if<std::string>(&kind_attribute->value);
    if (event_kind == nullptr)
        throw std::invalid_argument("SPC runtime event kind must be a string");

    const bool global_event =
        *event_kind == "continuation_lost" || *event_kind == "execution_reset";

    const auto source_name = event->provenance.empty()
        ? std::string{"spc-analysis"}
        : event->provenance[0].source;

    const auto add_optional_attribute = [&](
        const char* attribute_key,
        const char* feature_name,
        const char* fallback_detail,
        feature_availability voice_missing = feature_availability::unknown) {
        const attribute* item = find_spc_analysis_attribute(*event, attribute_key);
        if (item != nullptr) {
            analysis_feature feature = present_feature(
                feature_name,
                item->value,
                item->status,
                item->confidence,
                item->unit);
            feature.support_nodes.push_back(event_id);
            feature.provenance = event->provenance;
            features.add(std::move(feature));
            return;
        }

        features.add(unresolved_feature(
            feature_name,
            global_event ? feature_availability::not_applicable : voice_missing,
            global_event
                ? "global S-DSP execution event has no per-voice value for this feature"
                : fallback_detail,
            source_name));
    };

    add_optional_attribute(
        "physical_voice",
        "physical_voice",
        "this voice-local runtime observation does not carry a physical voice value");
    add_optional_attribute(
        "source_index",
        "source_index",
        "source identity is meaningful for the voice but was not observed on this runtime event");
    add_optional_attribute(
        "brr_address",
        "brr_address",
        "BRR address is meaningful for the voice but was not observed on this runtime event");
    add_optional_attribute(
        "pitch_rate",
        "device_native_pitch_rate",
        "device-native pitch rate was not observed on this runtime event");
    add_optional_attribute(
        "envelope_value",
        "device_native_envelope_value",
        "envelope value was not observed on this runtime event");
    add_optional_attribute(
        "key_on_delay",
        "device_native_key_on_delay",
        "KON delay is meaningful only when observed at the relevant runtime phase");
    add_optional_attribute(
        "noise_enabled",
        "noise_enabled",
        "noise state was not observed on this runtime event");

    std::optional<edge_id> episode_edge_id{};
    std::optional<node_id> episode_id{};

    const auto caused = graph.edges_from(event_id, edge_kind::causes);
    for (const edge* relation : caused) {
        const node* target = graph.find_node(relation->to);
        if (target != nullptr && target->kind == node_kind::voice_instance) {
            episode_edge_id = relation->id;
            episode_id = relation->to;
            break;
        }
    }
    if (!episode_id.has_value()) {
        const auto contributions = graph.edges_from(event_id, edge_kind::contributes_to);
        for (const edge* relation : contributions) {
            const node* target = graph.find_node(relation->to);
            if (target != nullptr && target->kind == node_kind::voice_instance) {
                episode_edge_id = relation->id;
                episode_id = relation->to;
                break;
            }
        }
    }

    if (episode_id.has_value()) {
        analysis_feature episode = present_feature(
            "physical_voice_episode_id",
            attribute_value{static_cast<std::uint64_t>(*episode_id)},
            evidence_status::derived,
            1.0,
            "node_id");
        episode.support_nodes.push_back(event_id);
        episode.support_nodes.push_back(*episode_id);
        if (episode_edge_id.has_value())
            episode.support_edges.push_back(*episode_edge_id);
        const edge* relation = episode_edge_id.has_value() ? graph.find_edge(*episode_edge_id) : nullptr;
        if (relation != nullptr)
            episode.provenance = relation->provenance;
        features.add(std::move(episode));
    } else {
        features.add(unresolved_feature(
            "physical_voice_episode_id",
            global_event ? feature_availability::not_applicable : feature_availability::unknown,
            global_event
                ? "global S-DSP execution event has no one physical voice episode"
                : "no bounded physical voice episode is linked to this runtime event",
            source_name));
    }

    const auto references = graph.edges_from(event_id, edge_kind::references);
    std::optional<node_id> sample_id{};
    std::optional<edge_id> sample_edge_id{};
    for (const edge* relation : references) {
        const node* target = graph.find_node(relation->to);
        if (target != nullptr && target->kind == node_kind::sample_buffer) {
            sample_id = relation->to;
            sample_edge_id = relation->id;
            break;
        }
    }

    if (sample_id.has_value()) {
        analysis_feature sample = present_feature(
            "runtime_sample_version_id",
            attribute_value{static_cast<std::uint64_t>(*sample_id)},
            evidence_status::derived,
            1.0,
            "node_id");
        sample.support_nodes.push_back(event_id);
        sample.support_nodes.push_back(*sample_id);
        if (sample_edge_id.has_value())
            sample.support_edges.push_back(*sample_edge_id);
        const edge* relation = sample_edge_id.has_value() ? graph.find_edge(*sample_edge_id) : nullptr;
        if (relation != nullptr)
            sample.provenance = relation->provenance;
        features.add(std::move(sample));
    } else {
        features.add(unresolved_feature(
            "runtime_sample_version_id",
            global_event ? feature_availability::not_applicable : feature_availability::unknown,
            global_event
                ? "global S-DSP execution event has no sample reference"
                : "no exact event-time BRR RAM version has been materialized for this runtime observation",
            source_name));
    }

    features.add(unresolved_feature(
        "sample_root_tuning",
        global_event ? feature_availability::not_applicable : feature_availability::unknown,
        global_event
            ? "global S-DSP execution event has no sample tuning semantics"
            : "runtime BRR/source identity does not by itself establish authored sample root tuning",
        source_name));
    features.add(unresolved_feature(
        "normalized_absolute_pitch",
        global_event ? feature_availability::not_applicable : feature_availability::unknown,
        global_event
            ? "global S-DSP execution event has no pitched-source semantics"
            : "S-DSP pitch rate does not establish absolute musical pitch without tuning/source continuity",
        source_name));
    features.add(unresolved_feature(
        "persistent_part_identity",
        global_event ? feature_availability::not_applicable : feature_availability::unknown,
        global_event
            ? "global S-DSP execution event is not a musical part observation"
            : "physical S-DSP voice continuity does not establish persistent musical-part identity",
        source_name));
    features.add(unresolved_feature(
        "original_driver_track",
        feature_availability::unavailable,
        "the current instrumented S-DSP runtime boundary does not expose validated driver-track identity",
        source_name));

    return features;
}

} // namespace gameaudio::spc
