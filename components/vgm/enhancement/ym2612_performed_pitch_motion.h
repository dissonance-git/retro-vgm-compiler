#pragma once

#include "genesis_pitch_motion_adapter.h"
#include "ym2612_episode_pitch_analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

struct ym2612_performed_pitch_motion_projection {
    std::vector<vgmtooling::model::pitch_motion_sample> samples;
    std::size_t nominal_sample_count = 0;
    std::size_t pre_episode_states_rebased = 0;
    std::optional<vgmtooling::model::node_id> invalidating_transition_id{};
    std::optional<vgmtooling::model::time_coordinate> invalidating_time{};
    bool static_operator_network_grounded = false;
    double confidence = 0.0;
    std::string detail;
};

inline const vgmtooling::model::attribute* find_ym2612_motion_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::optional<std::uint64_t> ym2612_motion_uint_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    const auto* item = find_ym2612_motion_attribute(value, key);
    if (item == nullptr)
        return std::nullopt;
    const auto* number = std::get_if<std::uint64_t>(&item->value);
    if (number == nullptr)
        return std::nullopt;
    return *number;
}

inline std::optional<std::string> ym2612_motion_string_attribute(
    const vgmtooling::model::node& value,
    const char* key) {
    const auto* item = find_ym2612_motion_attribute(value, key);
    if (item == nullptr)
        return std::nullopt;
    const auto* text = std::get_if<std::string>(&item->value);
    if (text == nullptr)
        return std::nullopt;
    return *text;
}

inline std::optional<std::size_t> ym2612_register_channel(
    std::uint8_t port,
    std::uint8_t reg) noexcept {
    const std::uint8_t local = static_cast<std::uint8_t>(reg & 0x03u);
    if (local >= 3u || port > 1u)
        return std::nullopt;
    return static_cast<std::size_t>(port) * 3u + local;
}

inline std::optional<std::size_t> ym2612_key_data_channel(
    std::uint8_t data) noexcept {
    const std::uint8_t code = static_cast<std::uint8_t>(data & 0x07u);
    if (code <= 2u)
        return static_cast<std::size_t>(code);
    if (code >= 4u && code <= 6u)
        return static_cast<std::size_t>(code - 1u);
    return std::nullopt;
}

inline bool ym2612_transition_targets_channel_register(
    std::uint8_t port,
    std::uint8_t reg,
    std::size_t channel) noexcept {
    const auto target = ym2612_register_channel(port, reg);
    return target.has_value() && *target == channel;
}

inline bool ym2612_transition_invalidates_static_pitch_network(
    const vgmtooling::model::node& transition,
    const ym2612_episode_synthesis_snapshot& snapshot) {
    using namespace vgmtooling::model;

    if (transition.kind != node_kind::trace_event ||
        transition.layer != semantic_layer::synthesis)
        return false;

    const auto family = ym2612_motion_string_attribute(transition, "device_family");
    const auto instance = ym2612_motion_uint_attribute(transition, "instance");
    const auto kind = ym2612_motion_string_attribute(transition, "transition_kind");
    if (!family.has_value() || *family != "YM2612" ||
        !instance.has_value() || *instance != snapshot.instance ||
        !kind.has_value() || *kind != "register_write") {
        return false;
    }

    const auto port_value = ym2612_motion_uint_attribute(transition, "port");
    const auto reg_value = ym2612_motion_uint_attribute(transition, "register");
    const auto data_value = ym2612_motion_uint_attribute(transition, "data");
    if (!port_value.has_value() || !reg_value.has_value() || !data_value.has_value() ||
        *port_value > 1u || *reg_value > 0xffu || *data_value > 0xffu) {
        return false;
    }

    const auto port = static_cast<std::uint8_t>(*port_value);
    const auto reg = static_cast<std::uint8_t>(*reg_value);
    const auto data = static_cast<std::uint8_t>(*data_value);

    // FNUM/BLOCK writes are the trajectory being projected, not invalidators.
    if ((reg >= 0xA0u && reg <= 0xA2u) ||
        (reg >= 0xA4u && reg <= 0xA6u)) {
        return false;
    }

    // DT/MULT changes alter operator frequency ratios. The current snapshot
    // model does not carry an exact post-onset operator-state trajectory, so a
    // write is an interpretation boundary even when it might be redundant.
    if (reg >= 0x30u && reg <= 0x3Fu &&
        ym2612_transition_targets_channel_register(port, reg, snapshot.channel_index)) {
        return true;
    }

    // Algorithm changes alter carrier topology; FMS/AMS writes can activate or
    // alter LFO pitch modulation. Both invalidate a static onset network.
    if (((reg >= 0xB0u && reg <= 0xB2u) ||
         (reg >= 0xB4u && reg <= 0xB6u)) &&
        ym2612_transition_targets_channel_register(port, reg, snapshot.channel_index)) {
        return true;
    }

    if (port == 0u && reg == 0x28u) {
        const auto target = ym2612_key_data_channel(data);
        return target.has_value() && *target == snapshot.channel_index;
    }

    // These global controls can change whether ordinary channel pitch remains
    // meaningful for the affected channel.
    if (port == 0u && reg == 0x27u && snapshot.channel_index == 2u)
        return true;
    if (port == 0u && reg == 0x2Bu && snapshot.channel_index == 5u)
        return true;
    if (port == 0u && reg == 0x22u && snapshot.channel.fms != 0u)
        return true;

    return false;
}

inline std::optional<std::uint64_t> ym2612_transition_source_offset(
    const vgmtooling::model::node& transition) noexcept {
    for (const auto& provenance : transition.provenance) {
        if (provenance.byte_offset.has_value())
            return provenance.byte_offset;
    }
    return std::nullopt;
}

inline std::optional<std::uint64_t> ym2612_snapshot_source_offset(
    const vgmtooling::model::node& snapshot) noexcept {
    for (const auto& provenance : snapshot.provenance) {
        if (provenance.byte_offset.has_value())
            return provenance.byte_offset;
    }
    return std::nullopt;
}

inline ym2612_performed_pitch_motion_projection project_ym2612_performed_pitch_motion(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id pitch_parameter_id,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    const auto nominal = project_genesis_pitch_motion(graph, pitch_parameter_id, clocks);
    if (nominal.samples.empty())
        throw std::invalid_argument("YM2612 performed-pitch motion requires at least one device-pitch state");

    const node* parameter = graph.find_node(pitch_parameter_id);
    if (parameter == nullptr)
        throw std::invalid_argument("YM2612 performed-pitch motion references an unknown pitch parameter");
    const auto family = ym2612_motion_string_attribute(*parameter, "device_family");
    if (!family.has_value() || *family != "YM2612")
        throw std::invalid_argument("YM2612 performed-pitch motion requires a YM2612 pitch parameter");

    const auto controls = graph.edges_from(pitch_parameter_id, edge_kind::controls);
    if (controls.size() != 1)
        throw std::invalid_argument("YM2612 pitch parameter must control exactly one physical episode");
    const node_id episode_id = controls.front()->to;
    const node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance ||
        !episode->active.has_value()) {
        throw std::invalid_argument("YM2612 performed-pitch motion requires a bounded physical episode");
    }

    const node* snapshot_node = nullptr;
    for (const edge* relation : graph.edges_to(episode_id, edge_kind::contributes_to)) {
        const node* candidate = graph.find_node(relation->from);
        if (candidate == nullptr || !is_ym2612_episode_synthesis_snapshot(*candidate))
            continue;
        if (snapshot_node != nullptr)
            throw std::invalid_argument("YM2612 performed-pitch motion found multiple onset synthesis snapshots");
        snapshot_node = candidate;
    }

    ym2612_performed_pitch_motion_projection result;
    result.nominal_sample_count = nominal.samples.size();
    if (snapshot_node == nullptr) {
        result.detail = "no source-backed YM2612 onset synthesis snapshot is available";
        return result;
    }

    const auto snapshot = read_ym2612_episode_synthesis_snapshot(*snapshot_node);
    const auto fundamental = infer_ym2612_snapshot_fundamental(snapshot, clocks);
    if (!fundamental.performed_pitch_frequency_hz.has_value() ||
        fundamental.performed_pitch_ambiguous) {
        result.confidence = fundamental.confidence;
        result.detail = fundamental.detail;
        return result;
    }

    const auto nominal_onset = ym2612_nominal_pitch_frequency_hz(
        snapshot.channel.fnum,
        snapshot.channel.block,
        clocks.ym2612_clock_hz);
    if (!nominal_onset.has_value() || !std::isfinite(*nominal_onset) || *nominal_onset <= 0.0)
        throw std::invalid_argument("YM2612 onset snapshot has no normalizable nominal channel pitch");

    const double performed_scale =
        *fundamental.performed_pitch_frequency_hz / *nominal_onset;
    if (!std::isfinite(performed_scale) || performed_scale <= 0.0)
        throw std::logic_error("YM2612 performed-pitch scale is invalid");

    result.static_operator_network_grounded = true;
    result.confidence = fundamental.confidence;

    const auto snapshot_offset = ym2612_snapshot_source_offset(*snapshot_node);
    const auto episode_start = episode->active->start;
    const auto episode_end = episode->active->end;

    const node* first_invalidator = nullptr;
    for (const auto& transition : graph.nodes()) {
        if (!transition.active.has_value() ||
            transition.active->start.domain != episode_start.domain ||
            transition.active->start.loop_iteration != episode_start.loop_iteration ||
            transition.active->start.tick_rate != episode_start.tick_rate)
            continue;
        const auto tick = transition.active->start.tick;
        if (tick < episode_start.tick)
            continue;
        if (episode_end.has_value() && tick >= episode_end->tick)
            continue;
        if (!ym2612_transition_invalidates_static_pitch_network(transition, snapshot))
            continue;

        // The onset snapshot already includes source writes through its own
        // source offset. Same-tick setup at or before that offset is antecedent
        // state, not a post-onset invalidation.
        if (tick == episode_start.tick && snapshot_offset.has_value()) {
            const auto transition_offset = ym2612_transition_source_offset(transition);
            if (transition_offset.has_value() && *transition_offset <= *snapshot_offset)
                continue;
        }

        if (first_invalidator == nullptr ||
            tick < first_invalidator->active->start.tick ||
            (tick == first_invalidator->active->start.tick && transition.id < first_invalidator->id)) {
            first_invalidator = &transition;
        }
    }

    if (first_invalidator != nullptr) {
        result.invalidating_transition_id = first_invalidator->id;
        result.invalidating_time = first_invalidator->active->start;
    }

    const double log2_scale = std::log2(performed_scale);
    for (auto sample : nominal.samples) {
        // The initial pitch value may have been established before key-on. Keep
        // its source node/provenance but move the performed state to the point
        // where the physical episode actually begins sounding.
        if (sample.time.domain == episode_start.domain &&
            sample.time.loop_iteration == episode_start.loop_iteration &&
            sample.time.tick_rate == episode_start.tick_rate &&
            sample.time.tick < episode_start.tick) {
            sample.time = episode_start;
            ++result.pre_episode_states_rebased;
        }
        if (sample.time.domain != episode_start.domain ||
            sample.time.loop_iteration != episode_start.loop_iteration ||
            sample.time.tick_rate != episode_start.tick_rate ||
            sample.time.tick < episode_start.tick)
            continue;
        if (episode_end.has_value() && sample.time.tick >= episode_end->tick)
            continue;
        if (result.invalidating_time.has_value() &&
            sample.time.tick >= result.invalidating_time->tick)
            continue;

        sample.log2_pitch_coordinate += log2_scale;
        sample.pitch_basis = "ym2612_static_operator_network_performed_frequency_hz";
        sample.interval_semantics = "log2_frequency_ratio_octaves";

        if (!result.samples.empty() && result.samples.back().time == sample.time)
            result.samples.back() = std::move(sample);
        else
            result.samples.push_back(std::move(sample));
    }

    if (result.invalidating_transition_id.has_value()) {
        result.detail =
            "performed-pitch trajectory is valid only through the first post-onset YM2612 state change that can alter operator ratios, carrier topology, key state, LFO pitch semantics, CH3 mode, or DAC semantics";
    } else {
        result.detail =
            "static source-backed YM2612 operator-network pitch interpretation scales the observed FNUM/BLOCK trajectory across the bounded physical episode";
    }
    return result;
}

inline std::optional<vgmtooling::model::pitch_motion_articulation_hypothesis>
analyze_ym2612_performed_pitch_motion(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id pitch_parameter_id,
    const genesis_pitch_clock_context& clocks,
    vgmtooling::model::pitch_motion_analysis_policy policy = {}) {
    const auto projection = project_ym2612_performed_pitch_motion(
        graph,
        pitch_parameter_id,
        clocks);
    if (!projection.static_operator_network_grounded || projection.samples.size() < 2)
        return std::nullopt;
    return vgmtooling::model::analyze_in_episode_pitch_motion(
        projection.samples,
        projection.confidence,
        policy);
}

} // namespace gameaudio::vgm
