#pragma once

#include "genesis_performance_adapter.h"
#include "../../../model/analysis_feature.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace gameaudio::vgm {

inline const vgmtooling::model::attribute* find_genesis_analysis_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline vgmtooling::model::analysis_feature genesis_feature_from_attribute(
    const vgmtooling::model::node& event,
    vgmtooling::model::node_id event_id,
    const char* attribute_key,
    const char* feature_name) {
    using namespace vgmtooling::model;

    const attribute* item = find_genesis_analysis_attribute(event, attribute_key);
    if (item == nullptr)
        throw std::invalid_argument("required Genesis analysis attribute is missing");

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

inline vgmtooling::model::analysis_feature_set extract_genesis_performance_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id) {
    using namespace vgmtooling::model;

    const node* event = graph.find_node(event_id);
    if (event == nullptr)
        throw std::invalid_argument("Genesis analysis feature extraction references an unknown node");
    if (event->kind != node_kind::musical_event ||
        event->layer != semantic_layer::musical_performance) {
        throw std::invalid_argument("Genesis analysis features require a musical-performance event");
    }

    analysis_feature_set features;
    features.add(genesis_feature_from_attribute(*event, event_id, "event_kind", "event_kind"));
    features.add(genesis_feature_from_attribute(*event, event_id, "device_family", "device_family"));
    features.add(genesis_feature_from_attribute(*event, event_id, "instance", "device_instance"));
    features.add(genesis_feature_from_attribute(*event, event_id, "physical_channel", "physical_channel"));

    const attribute* family_attribute = find_genesis_analysis_attribute(*event, "device_family");
    const auto* family = family_attribute == nullptr
        ? nullptr
        : std::get_if<std::string>(&family_attribute->value);
    if (family == nullptr)
        throw std::invalid_argument("Genesis device-family analysis attribute must be a string");

    const attribute* pitch_code = find_genesis_analysis_attribute(*event, "device_pitch_code");
    if (pitch_code != nullptr) {
        analysis_feature feature = present_feature(
            "device_native_pitch_code",
            pitch_code->value,
            pitch_code->status,
            pitch_code->confidence,
            pitch_code->unit.empty() ? "device_native" : pitch_code->unit);
        feature.support_nodes.push_back(event_id);
        feature.provenance = event->provenance;
        features.add(std::move(feature));
    } else {
        features.add(unresolved_feature(
            "device_native_pitch_code",
            feature_availability::unknown,
            "this conservative performance event does not carry a device-native pitch code",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    const attribute* pitch_block = find_genesis_analysis_attribute(*event, "device_pitch_block");
    if (pitch_block != nullptr) {
        analysis_feature feature = present_feature(
            "device_native_pitch_block",
            pitch_block->value,
            pitch_block->status,
            pitch_block->confidence,
            pitch_block->unit.empty() ? "device_native" : pitch_block->unit);
        feature.support_nodes.push_back(event_id);
        feature.provenance = event->provenance;
        features.add(std::move(feature));
    } else if (*family == "SN76489") {
        features.add(unresolved_feature(
            "device_native_pitch_block",
            feature_availability::not_applicable,
            "SN76489 tone periods do not use the YM2612 block field",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    } else {
        features.add(unresolved_feature(
            "device_native_pitch_block",
            feature_availability::unknown,
            "a block value is meaningful for ordinary YM2612 pitch but is not established on this event",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    const attribute* gate_or_level = find_genesis_analysis_attribute(*event, "gate_or_level");
    if (gate_or_level != nullptr) {
        analysis_feature feature = present_feature(
            "device_gate_or_level",
            gate_or_level->value,
            gate_or_level->status,
            gate_or_level->confidence,
            gate_or_level->unit.empty() ? "device_native" : gate_or_level->unit);
        feature.support_nodes.push_back(event_id);
        feature.provenance = event->provenance;
        features.add(std::move(feature));
    } else {
        features.add(unresolved_feature(
            "device_gate_or_level",
            feature_availability::unknown,
            "the conservative event does not expose a gate/level value",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    const auto realizations = graph.edges_from(event_id, edge_kind::realizes);
    if (realizations.size() == 1 &&
        graph.find_node(realizations[0]->to) != nullptr &&
        graph.find_node(realizations[0]->to)->kind == node_kind::voice_instance) {
        analysis_feature episode = present_feature(
            "physical_voice_episode_id",
            attribute_value{static_cast<std::uint64_t>(realizations[0]->to)},
            evidence_status::derived,
            1.0,
            "node_id");
        episode.support_nodes.push_back(event_id);
        episode.support_nodes.push_back(realizations[0]->to);
        episode.support_edges.push_back(realizations[0]->id);
        episode.provenance = realizations[0]->provenance;
        features.add(std::move(episode));
    } else {
        features.add(unresolved_feature(
            "physical_voice_episode_id",
            feature_availability::unknown,
            "no unique bounded physical voice episode is established for this performance event",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    features.add(unresolved_feature(
        "persistent_part_identity",
        feature_availability::unknown,
        "a bounded Genesis physical episode does not establish persistent musical-part identity",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    features.add(unresolved_feature(
        "normalized_absolute_pitch",
        feature_availability::unknown,
        "device-native pitch is preserved, but this extractor does not invent an authored or normalized absolute pitch",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    features.add(unresolved_feature(
        "original_driver_track",
        feature_availability::unavailable,
        "this VGM/device-derived performance boundary does not expose validated original driver-track identity",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    features.add(unresolved_feature(
        "sample_identity",
        feature_availability::not_applicable,
        "the conservative Genesis events currently admitted here are ordinary FM or PSG tone activity, not sample playback",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));

    return features;
}

} // namespace gameaudio::vgm
