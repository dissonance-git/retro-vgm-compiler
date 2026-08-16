#pragma once

#include "genesis_nominal_pitch.h"
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
    const char* feature_name,
    vgmtooling::model::semantic_layer claim_layer) {
    using namespace vgmtooling::model;

    const attribute* item = find_genesis_analysis_attribute(event, attribute_key);
    if (item == nullptr)
        throw std::invalid_argument("required Genesis analysis attribute is missing");

    analysis_feature feature = present_feature(
        feature_name,
        claim_layer,
        item->value,
        item->status,
        item->confidence,
        item->unit);
    feature.support_nodes.push_back(event_id);
    feature.provenance = event.provenance;
    return feature;
}

inline std::optional<double> derive_genesis_nominal_pitch_frequency_hz(
    const vgmtooling::model::node& event,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    const attribute* family_attribute = find_genesis_analysis_attribute(event, "device_family");
    const attribute* pitch_attribute = find_genesis_analysis_attribute(event, "device_pitch_code");
    if (family_attribute == nullptr || pitch_attribute == nullptr)
        return std::nullopt;

    const auto* family = std::get_if<std::string>(&family_attribute->value);
    const auto* pitch = std::get_if<std::uint64_t>(&pitch_attribute->value);
    if (family == nullptr || pitch == nullptr)
        return std::nullopt;

    if (*family == "YM2612") {
        const attribute* block_attribute = find_genesis_analysis_attribute(event, "device_pitch_block");
        if (block_attribute == nullptr)
            return std::nullopt;
        const auto* block = std::get_if<std::uint64_t>(&block_attribute->value);
        if (block == nullptr || *pitch > 0x07ffu || *block > 7u)
            return std::nullopt;
        return ym2612_nominal_pitch_frequency_hz(
            static_cast<std::uint16_t>(*pitch),
            static_cast<std::uint8_t>(*block),
            clocks.ym2612_clock_hz);
    }

    if (*family == "SN76489") {
        if (*pitch > 0x03ffu)
            return std::nullopt;
        return sn76489_nominal_pitch_frequency_hz(
            static_cast<std::uint16_t>(*pitch),
            clocks.sn76489_clock_hz);
    }

    return std::nullopt;
}

inline vgmtooling::model::analysis_feature_set extract_genesis_performance_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id,
    const genesis_pitch_clock_context* pitch_clocks = nullptr) {
    using namespace vgmtooling::model;

    const node* event = graph.find_node(event_id);
    if (event == nullptr)
        throw std::invalid_argument("Genesis analysis feature extraction references an unknown node");
    if (event->kind != node_kind::musical_event ||
        event->layer != semantic_layer::musical_performance) {
        throw std::invalid_argument("Genesis analysis features require a musical-performance event");
    }

    analysis_feature_set features;
    features.add(genesis_feature_from_attribute(
        *event, event_id, "event_kind", "event_kind", semantic_layer::musical_performance));
    features.add(genesis_feature_from_attribute(
        *event, event_id, "device_family", "device_family", semantic_layer::synthesis));
    features.add(genesis_feature_from_attribute(
        *event, event_id, "instance", "device_instance", semantic_layer::synthesis));
    features.add(genesis_feature_from_attribute(
        *event, event_id, "physical_channel", "physical_channel", semantic_layer::synthesis));

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
            semantic_layer::synthesis,
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
            semantic_layer::synthesis,
            feature_availability::unknown,
            "this conservative performance event does not carry a device-native pitch code",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    const attribute* pitch_block = find_genesis_analysis_attribute(*event, "device_pitch_block");
    if (pitch_block != nullptr) {
        analysis_feature feature = present_feature(
            "device_native_pitch_block",
            semantic_layer::synthesis,
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
            semantic_layer::synthesis,
            feature_availability::not_applicable,
            "SN76489 tone periods do not use the YM2612 block field",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    } else {
        features.add(unresolved_feature(
            "device_native_pitch_block",
            semantic_layer::synthesis,
            feature_availability::unknown,
            "a block value is meaningful for ordinary YM2612 pitch but is not established on this event",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    std::optional<double> nominal_frequency;
    if (pitch_clocks != nullptr) {
        nominal_frequency = derive_genesis_nominal_pitch_frequency_hz(*event, *pitch_clocks);
        if (nominal_frequency.has_value()) {
            analysis_feature feature = present_feature(
                "device_nominal_pitch_frequency_hz",
                semantic_layer::synthesis,
                attribute_value{*nominal_frequency},
                evidence_status::derived,
                1.0,
                "Hz");
            feature.support_nodes.push_back(event_id);
            feature.provenance = event->provenance;
            feature.provenance.push_back({
                evidence_status::exact,
                1.0,
                pitch_clocks->source.empty() ? "genesis-pitch-clock-context" : pitch_clocks->source,
                std::nullopt,
                "source-relative device clock used to derive the nominal channel/tone frequency; this is not a MIDI note, chord tone, or perceptual pitch",
            });
            features.add(std::move(feature));
        } else {
            features.add(unresolved_feature(
                "device_nominal_pitch_frequency_hz",
                semantic_layer::synthesis,
                feature_availability::unknown,
                "the supplied clock context is insufficient or incompatible with this device-native pitch state",
                pitch_clocks->source.empty() ? "genesis-pitch-clock-context" : pitch_clocks->source));
        }
    } else {
        features.add(unresolved_feature(
            "device_nominal_pitch_frequency_hz",
            semantic_layer::synthesis,
            feature_availability::unknown,
            "device-native pitch is preserved but no source-relative chip clock was supplied for frequency normalization",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    const attribute* gate_or_level = find_genesis_analysis_attribute(*event, "gate_or_level");
    if (gate_or_level != nullptr) {
        analysis_feature feature = present_feature(
            "device_gate_or_level",
            semantic_layer::synthesis,
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
            semantic_layer::synthesis,
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
            semantic_layer::synthesis,
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
            semantic_layer::synthesis,
            feature_availability::unknown,
            "no unique bounded physical voice episode is established for this performance event",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    features.add(unresolved_feature(
        "persistent_part_identity",
        semantic_layer::musical_performance,
        feature_availability::unknown,
        "a bounded Genesis physical episode does not establish persistent musical-part identity",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));

    if (*family == "SN76489" && nominal_frequency.has_value()) {
        analysis_feature performed = present_feature(
            "performed_pitch_frequency_hz",
            semantic_layer::musical_performance,
            attribute_value{*nominal_frequency},
            evidence_status::derived,
            1.0,
            "Hz");
        performed.support_nodes.push_back(event_id);
        performed.provenance = event->provenance;
        performed.provenance.push_back({
            evidence_status::derived,
            1.0,
            pitch_clocks != nullptr && !pitch_clocks->source.empty()
                ? pitch_clocks->source
                : "genesis-sn76489-tone-analysis",
            std::nullopt,
            "ordinary SN76489 tone activity is a direct square-wave oscillator: the source-relative tone period and clock establish its performed fundamental frequency; persistent-part identity, musical role, and heard/perceptual interpretation remain separate claims",
        });
        features.add(std::move(performed));
    } else {
        features.add(unresolved_feature(
            "performed_pitch_frequency_hz",
            semantic_layer::musical_performance,
            feature_availability::unknown,
            *family == "YM2612"
                ? "YM2612 performed pitch requires operator-network analysis beyond nominal FNUM/BLOCK frequency"
                : "performed pitch remains unresolved for this Genesis event/device state",
            event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    }

    features.add(unresolved_feature(
        "original_driver_track",
        semantic_layer::driver_execution,
        feature_availability::unavailable,
        "this VGM/device-derived performance boundary does not expose validated original driver-track identity",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));
    features.add(unresolved_feature(
        "sample_identity",
        semantic_layer::synthesis,
        feature_availability::not_applicable,
        "the conservative Genesis events currently admitted here are ordinary FM or PSG tone activity, not sample playback",
        event->provenance.empty() ? "genesis-analysis" : event->provenance[0].source));

    return features;
}

} // namespace gameaudio::vgm
