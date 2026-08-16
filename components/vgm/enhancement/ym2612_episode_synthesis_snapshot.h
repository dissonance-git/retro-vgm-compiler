#pragma once

#include "genesis_state.h"
#include "../../../model/musical_execution_graph.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameaudio::vgm {

struct ym2612_episode_synthesis_snapshot {
    std::size_t instance = 0;
    std::size_t channel_index = 0;
    ym2612_channel_state channel{};
    bool lfo_enabled = false;
    std::uint8_t lfo_frequency = 0;
};

inline ym2612_episode_synthesis_snapshot capture_ym2612_episode_synthesis_snapshot(
    const ym2612_state& state,
    std::size_t instance,
    std::size_t channel_index) {
    if (channel_index >= state.channels.size())
        throw std::invalid_argument("YM2612 synthesis snapshot references an invalid channel");
    return {
        instance,
        channel_index,
        state.channels[channel_index],
        state.lfo_enabled,
        state.lfo_frequency,
    };
}

inline vgmtooling::model::node_id add_ym2612_episode_synthesis_snapshot(
    vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id physical_episode_id,
    const ym2612_episode_synthesis_snapshot& snapshot,
    const vgmtooling::model::time_coordinate& onset,
    std::string source,
    std::optional<std::uint64_t> source_offset = std::nullopt) {
    using namespace vgmtooling::model;

    const node* episode = graph.find_node(physical_episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("YM2612 synthesis snapshot requires a physical voice episode");
    if (source.empty())
        throw std::invalid_argument("YM2612 synthesis snapshot requires provenance source");

    node state;
    state.kind = node_kind::synthesis_object;
    state.layer = semantic_layer::synthesis;
    state.flow = flow_kind::value;
    state.label = "YM2612 episode-onset synthesis state";
    state.active = time_span{onset, std::nullopt};
    state.attributes.push_back({
        "identity_scope",
        std::string{"ym2612_episode_synthesis_snapshot"},
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "instance",
        static_cast<std::uint64_t>(snapshot.instance),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "physical_channel",
        static_cast<std::uint64_t>(snapshot.channel_index),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "fnum",
        static_cast<std::uint64_t>(snapshot.channel.fnum),
        evidence_status::derived,
        1.0,
        "device_native",
    });
    state.attributes.push_back({
        "block",
        static_cast<std::uint64_t>(snapshot.channel.block),
        evidence_status::derived,
        1.0,
        "device_native",
    });
    state.attributes.push_back({
        "algorithm",
        static_cast<std::uint64_t>(snapshot.channel.algorithm),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "feedback",
        static_cast<std::uint64_t>(snapshot.channel.feedback),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "operator_key_mask",
        static_cast<std::uint64_t>(snapshot.channel.operator_key_mask),
        evidence_status::derived,
        1.0,
        "bitmask",
    });
    state.attributes.push_back({
        "ams",
        static_cast<std::uint64_t>(snapshot.channel.ams),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "fms",
        static_cast<std::uint64_t>(snapshot.channel.fms),
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "lfo_enabled",
        snapshot.lfo_enabled,
        evidence_status::derived,
        1.0,
        "",
    });
    state.attributes.push_back({
        "lfo_frequency",
        static_cast<std::uint64_t>(snapshot.lfo_frequency),
        evidence_status::derived,
        1.0,
        "device_native",
    });

    for (std::size_t op_index = 0; op_index < snapshot.channel.operators.size(); ++op_index) {
        const auto& op = snapshot.channel.operators[op_index];
        const std::string prefix = "op" + std::to_string(op_index + 1) + "_";
        state.attributes.push_back({
            prefix + "multiple",
            static_cast<std::uint64_t>(op.multiple),
            evidence_status::derived,
            1.0,
            "device_native",
        });
        state.attributes.push_back({
            prefix + "detune",
            static_cast<std::uint64_t>(op.detune),
            evidence_status::derived,
            1.0,
            "device_native",
        });
        state.attributes.push_back({
            prefix + "total_level",
            static_cast<std::uint64_t>(op.total_level),
            evidence_status::derived,
            1.0,
            "device_native",
        });
    }

    state.provenance.push_back({
        evidence_status::derived,
        1.0,
        std::move(source),
        source_offset,
        "episode-onset YM2612 synthesis state rebuilt from the exact observed VGM register history; cache of source-backed state, not an authored instrument identity",
    });
    const node_id snapshot_id = graph.add_node(std::move(state));

    edge contribution;
    contribution.kind = edge_kind::contributes_to;
    contribution.from = snapshot_id;
    contribution.to = physical_episode_id;
    contribution.active = time_span{onset, std::nullopt};
    contribution.attributes.push_back({
        "support_role",
        std::string{"episode_onset_synthesis_state"},
        evidence_status::derived,
        1.0,
        "",
    });
    graph.add_edge(std::move(contribution));
    return snapshot_id;
}

} // namespace gameaudio::vgm
