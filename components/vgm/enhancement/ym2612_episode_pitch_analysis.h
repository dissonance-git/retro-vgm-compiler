#pragma once

#include "ym2612_episode_synthesis_snapshot.h"
#include "ym2612_fundamental_hypothesis.h"
#include "../../../model/analysis_feature.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameaudio::vgm {

inline const vgmtooling::model::attribute* find_ym2612_snapshot_attribute(
    const vgmtooling::model::node& value,
    const std::string& key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::uint64_t required_ym2612_snapshot_uint(
    const vgmtooling::model::node& value,
    const std::string& key) {
    const auto* item = find_ym2612_snapshot_attribute(value, key);
    if (item == nullptr)
        throw std::invalid_argument("YM2612 synthesis snapshot is missing attribute: " + key);
    const auto* number = std::get_if<std::uint64_t>(&item->value);
    if (number == nullptr)
        throw std::invalid_argument("YM2612 synthesis snapshot attribute is not unsigned: " + key);
    return *number;
}

inline bool required_ym2612_snapshot_bool(
    const vgmtooling::model::node& value,
    const std::string& key) {
    const auto* item = find_ym2612_snapshot_attribute(value, key);
    if (item == nullptr)
        throw std::invalid_argument("YM2612 synthesis snapshot is missing attribute: " + key);
    const auto* flag = std::get_if<bool>(&item->value);
    if (flag == nullptr)
        throw std::invalid_argument("YM2612 synthesis snapshot attribute is not boolean: " + key);
    return *flag;
}

inline bool is_ym2612_episode_synthesis_snapshot(
    const vgmtooling::model::node& value) noexcept {
    if (value.kind != vgmtooling::model::node_kind::synthesis_object ||
        value.layer != vgmtooling::model::semantic_layer::synthesis) {
        return false;
    }
    const auto* item = find_ym2612_snapshot_attribute(value, "identity_scope");
    if (item == nullptr)
        return false;
    const auto* text = std::get_if<std::string>(&item->value);
    return text != nullptr && *text == "ym2612_episode_synthesis_snapshot";
}

inline ym2612_episode_synthesis_snapshot read_ym2612_episode_synthesis_snapshot(
    const vgmtooling::model::node& value) {
    if (!is_ym2612_episode_synthesis_snapshot(value))
        throw std::invalid_argument("node is not a YM2612 episode synthesis snapshot");

    ym2612_episode_synthesis_snapshot snapshot;
    snapshot.instance = static_cast<std::size_t>(required_ym2612_snapshot_uint(value, "instance"));
    snapshot.channel_index = static_cast<std::size_t>(required_ym2612_snapshot_uint(value, "physical_channel"));
    if (snapshot.instance > 1 || snapshot.channel_index > 5)
        throw std::invalid_argument("YM2612 synthesis snapshot has invalid device coordinates");

    auto& channel = snapshot.channel;
    channel.fnum = static_cast<std::uint16_t>(required_ym2612_snapshot_uint(value, "fnum"));
    channel.block = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "block"));
    channel.algorithm = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "algorithm"));
    channel.feedback = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "feedback"));
    channel.operator_key_mask = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "operator_key_mask"));
    channel.key_on = channel.operator_key_mask != 0;
    channel.ams = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "ams"));
    channel.fms = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "fms"));
    snapshot.lfo_enabled = required_ym2612_snapshot_bool(value, "lfo_enabled");
    snapshot.lfo_frequency = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, "lfo_frequency"));

    if (channel.fnum > 0x07ffu || channel.block > 7u || channel.algorithm > 7u ||
        channel.feedback > 7u || channel.ams > 3u || channel.fms > 7u ||
        snapshot.lfo_frequency > 7u) {
        throw std::invalid_argument("YM2612 synthesis snapshot contains out-of-range channel state");
    }

    for (std::size_t op_index = 0; op_index < channel.operators.size(); ++op_index) {
        auto& op = channel.operators[op_index];
        const std::string prefix = "op" + std::to_string(op_index + 1) + "_";
        op.multiple = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, prefix + "multiple"));
        op.detune = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, prefix + "detune"));
        op.total_level = static_cast<std::uint8_t>(required_ym2612_snapshot_uint(value, prefix + "total_level"));
        if (op.multiple > 15u || op.detune > 7u || op.total_level > 127u)
            throw std::invalid_argument("YM2612 synthesis snapshot contains out-of-range operator state");
    }
    return snapshot;
}

inline ym2612_fundamental_hypothesis infer_ym2612_snapshot_fundamental(
    const ym2612_episode_synthesis_snapshot& snapshot,
    const genesis_pitch_clock_context& clocks) {
    ym2612_state state;
    state.channels[snapshot.channel_index] = snapshot.channel;
    state.lfo_enabled = snapshot.lfo_enabled;
    state.lfo_frequency = snapshot.lfo_frequency;
    return infer_ym2612_fundamental_hypothesis(
        state,
        snapshot.channel_index,
        clocks);
}

inline vgmtooling::model::analysis_feature_set extract_ym2612_episode_pitch_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id physical_episode_id,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    const node* episode = graph.find_node(physical_episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("YM2612 episode pitch analysis requires a physical voice episode");

    const node* snapshot_node = nullptr;
    const edge* snapshot_edge = nullptr;
    for (const edge* relation : graph.edges_to(physical_episode_id, edge_kind::contributes_to)) {
        const node* candidate = graph.find_node(relation->from);
        if (candidate == nullptr || !is_ym2612_episode_synthesis_snapshot(*candidate))
            continue;
        if (snapshot_node != nullptr) {
            analysis_feature_set ambiguous;
            ambiguous.add(unresolved_feature(
                "fm_network_periodicity_frequency_hz",
                semantic_layer::synthesis,
                feature_availability::unknown,
                "multiple YM2612 episode-onset synthesis snapshots contribute to one physical episode",
                "ym2612-episode-pitch-analysis"));
            ambiguous.add(unresolved_feature(
                "performed_pitch_frequency_hz",
                semantic_layer::musical_performance,
                feature_availability::unknown,
                "multiple YM2612 episode-onset synthesis snapshots contribute to one physical episode",
                "ym2612-episode-pitch-analysis"));
            return ambiguous;
        }
        snapshot_node = candidate;
        snapshot_edge = relation;
    }

    analysis_feature_set features;
    if (snapshot_node == nullptr || snapshot_edge == nullptr) {
        features.add(unresolved_feature(
            "fm_network_periodicity_frequency_hz",
            semantic_layer::synthesis,
            feature_availability::unavailable,
            "no source-backed YM2612 episode-onset synthesis snapshot is attached to this physical episode",
            "ym2612-episode-pitch-analysis"));
        features.add(unresolved_feature(
            "performed_pitch_frequency_hz",
            semantic_layer::musical_performance,
            feature_availability::unavailable,
            "operator-aware performed-pitch analysis requires a source-backed YM2612 episode synthesis snapshot",
            "ym2612-episode-pitch-analysis"));
        return features;
    }

    const auto snapshot = read_ym2612_episode_synthesis_snapshot(*snapshot_node);
    const auto hypothesis = infer_ym2612_snapshot_fundamental(snapshot, clocks);

    if (hypothesis.network_periodicity_frequency_hz.has_value()) {
        analysis_feature periodicity = present_feature(
            "fm_network_periodicity_frequency_hz",
            semantic_layer::synthesis,
            attribute_value{*hypothesis.network_periodicity_frequency_hz},
            evidence_status::hypothesis,
            hypothesis.confidence,
            "Hz");
        periodicity.support_nodes = {snapshot_node->id, physical_episode_id};
        periodicity.support_edges = {snapshot_edge->id};
        periodicity.provenance = snapshot_node->provenance;
        periodicity.provenance.push_back({
            evidence_status::hypothesis,
            hypothesis.confidence,
            "ym2612-operator-network-analysis",
            std::nullopt,
            hypothesis.detail,
        });
        features.add(std::move(periodicity));
    } else {
        analysis_feature periodicity = unresolved_feature(
            "fm_network_periodicity_frequency_hz",
            semantic_layer::synthesis,
            feature_availability::unknown,
            hypothesis.detail,
            "ym2612-operator-network-analysis");
        periodicity.support_nodes = {snapshot_node->id, physical_episode_id};
        periodicity.support_edges = {snapshot_edge->id};
        features.add(std::move(periodicity));
    }

    if (hypothesis.performed_pitch_frequency_hz.has_value() &&
        !hypothesis.performed_pitch_ambiguous) {
        analysis_feature performed = present_feature(
            "performed_pitch_frequency_hz",
            semantic_layer::musical_performance,
            attribute_value{*hypothesis.performed_pitch_frequency_hz},
            evidence_status::hypothesis,
            hypothesis.confidence,
            "Hz");
        performed.support_nodes = {snapshot_node->id, physical_episode_id};
        performed.support_edges = {snapshot_edge->id};
        performed.provenance = snapshot_node->provenance;
        performed.provenance.push_back({
            evidence_status::hypothesis,
            hypothesis.confidence,
            "ym2612-operator-network-analysis",
            std::nullopt,
            hypothesis.detail,
        });
        features.add(std::move(performed));
    } else {
        analysis_feature performed = unresolved_feature(
            "performed_pitch_frequency_hz",
            semantic_layer::musical_performance,
            feature_availability::unknown,
            hypothesis.detail,
            "ym2612-operator-network-analysis");
        performed.support_nodes = {snapshot_node->id, physical_episode_id};
        performed.support_edges = {snapshot_edge->id};
        features.add(std::move(performed));
    }

    return features;
}

} // namespace gameaudio::vgm
