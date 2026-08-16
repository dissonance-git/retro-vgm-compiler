#pragma once

#include "genesis_nominal_pitch.h"
#include "../../../model/pitch_motion_articulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

struct genesis_pitch_motion_projection {
    std::vector<vgmtooling::model::pitch_motion_sample> samples;
    std::size_t raw_pitch_support_count = 0;
    std::size_t same_tick_states_coalesced = 0;
    bool nominal_device_pitch_only = true;
};

inline const vgmtooling::model::attribute* find_attribute(
    const std::vector<vgmtooling::model::attribute>& attributes,
    const char* key) noexcept {
    for (const auto& item : attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::string required_string_attribute(
    const vgmtooling::model::node& value,
    const char* key) {
    const auto* item = find_attribute(value.attributes, key);
    if (item == nullptr)
        throw std::invalid_argument(std::string{"missing required attribute: "} + key);
    const auto* text = std::get_if<std::string>(&item->value);
    if (text == nullptr)
        throw std::invalid_argument(std::string{"attribute is not a string: "} + key);
    return *text;
}

inline std::uint64_t required_uint_attribute(
    const vgmtooling::model::edge& value,
    const char* key) {
    const auto* item = find_attribute(value.attributes, key);
    if (item == nullptr)
        throw std::invalid_argument(std::string{"missing required edge attribute: "} + key);
    const auto* number = std::get_if<std::uint64_t>(&item->value);
    if (number == nullptr)
        throw std::invalid_argument(std::string{"edge attribute is not an unsigned integer: "} + key);
    return *number;
}

inline genesis_pitch_motion_projection project_genesis_pitch_motion(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id pitch_parameter_id,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    if (clocks.source.empty())
        throw std::invalid_argument("Genesis pitch-motion projection requires clock provenance");
    const node* parameter = graph.find_node(pitch_parameter_id);
    if (parameter == nullptr || parameter->kind != node_kind::parameter ||
        parameter->layer != semantic_layer::musical_performance) {
        throw std::invalid_argument("Genesis pitch-motion projection requires a performance pitch parameter");
    }
    if (required_string_attribute(*parameter, "parameter_kind") != "pitch" ||
        required_string_attribute(*parameter, "representation") != "device_native") {
        throw std::invalid_argument("Genesis pitch-motion projection requires a device-native pitch parameter");
    }
    const std::string family = required_string_attribute(*parameter, "device_family");

    const auto controls = graph.edges_from(pitch_parameter_id, edge_kind::controls);
    if (controls.size() != 1) {
        throw std::invalid_argument("Genesis pitch parameter must control exactly one physical episode");
    }
    const node_id episode_id = controls.front()->to;
    const node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("Genesis pitch parameter controls an invalid physical episode");

    auto supports = graph.edges_to(pitch_parameter_id, edge_kind::contributes_to);
    std::sort(supports.begin(), supports.end(), [](const edge* first, const edge* second) {
        if (!first->active.has_value() || !second->active.has_value())
            return first->id < second->id;
        if (first->active->start.tick != second->active->start.tick)
            return first->active->start.tick < second->active->start.tick;
        return first->id < second->id;
    });

    genesis_pitch_motion_projection result;
    result.raw_pitch_support_count = supports.size();
    for (const edge* support : supports) {
        if (!support->active.has_value())
            throw std::invalid_argument("Genesis pitch support is missing a time coordinate");
        const std::uint64_t pitch_code = required_uint_attribute(*support, "device_pitch_code");

        std::optional<double> frequency;
        if (family == "YM2612") {
            const std::uint64_t block = required_uint_attribute(*support, "device_pitch_block");
            if (pitch_code > 0x07ffu || block > 7u)
                throw std::invalid_argument("invalid YM2612 pitch state in performance control");
            frequency = ym2612_nominal_pitch_frequency_hz(
                static_cast<std::uint16_t>(pitch_code),
                static_cast<std::uint8_t>(block),
                clocks.ym2612_clock_hz);
        } else if (family == "SN76489") {
            if (pitch_code > 0x03ffu)
                throw std::invalid_argument("invalid SN76489 pitch state in performance control");
            frequency = sn76489_nominal_pitch_frequency_hz(
                static_cast<std::uint16_t>(pitch_code),
                clocks.sn76489_clock_hz);
        } else {
            throw std::invalid_argument("unsupported Genesis pitch-control device family");
        }
        if (!frequency.has_value() || !std::isfinite(*frequency) || *frequency <= 0.0)
            throw std::invalid_argument("Genesis clock context cannot normalize this pitch state");

        pitch_motion_sample sample;
        sample.source_node = support->from;
        sample.physical_episode_id = episode_id;
        sample.time = support->active->start;
        sample.log2_pitch_coordinate = std::log2(*frequency);
        sample.pitch_basis = "absolute_nominal_device_frequency_hz";
        sample.interval_semantics = "log2_frequency_ratio_octaves";

        // VGM register writes without an intervening wait share one source tick.
        // Preserve all raw transitions in the graph, but expose only the final
        // state at that tick to higher pitch-motion analysis. This prevents an
        // intermediate half-updated FNUM/BLOCK pair from becoming a fake note.
        if (!result.samples.empty() && result.samples.back().time == sample.time) {
            result.samples.back() = std::move(sample);
            ++result.same_tick_states_coalesced;
        } else {
            result.samples.push_back(std::move(sample));
        }
    }
    return result;
}

} // namespace gameaudio::vgm
