#pragma once

#include "genesis_performance_adapter.h"
#include "../../../model/persistent_part_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

inline const vgmtooling::model::attribute* find_genesis_part_attribute(
    const vgmtooling::model::node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline std::uint64_t genesis_fnv1a_byte(std::uint64_t hash, std::uint8_t value) noexcept {
    constexpr std::uint64_t prime = 1099511628211ull;
    hash ^= value;
    hash *= prime;
    return hash;
}

inline std::uint64_t ym2612_program_fingerprint(
    const ym2612_channel_state& channel) noexcept {
    // Pitch, key state and stereo routing are deliberately excluded.  The
    // fingerprint describes the pitch-invariant programmed FM instrument at
    // the channel boundary, not the current note or presentation route.
    std::uint64_t hash = 1469598103934665603ull;
    hash = genesis_fnv1a_byte(hash, channel.algorithm);
    hash = genesis_fnv1a_byte(hash, channel.feedback);
    hash = genesis_fnv1a_byte(hash, channel.ams);
    hash = genesis_fnv1a_byte(hash, channel.fms);
    for (const auto& op : channel.operators) {
        hash = genesis_fnv1a_byte(hash, op.detune);
        hash = genesis_fnv1a_byte(hash, op.multiple);
        hash = genesis_fnv1a_byte(hash, op.total_level);
        hash = genesis_fnv1a_byte(hash, op.key_scale);
        hash = genesis_fnv1a_byte(hash, op.attack_rate);
        hash = genesis_fnv1a_byte(hash, op.amplitude_modulation ? 1u : 0u);
        hash = genesis_fnv1a_byte(hash, op.decay_rate);
        hash = genesis_fnv1a_byte(hash, op.sustain_rate);
        hash = genesis_fnv1a_byte(hash, op.sustain_level);
        hash = genesis_fnv1a_byte(hash, op.release_rate);
        hash = genesis_fnv1a_byte(hash, op.ssg_eg);
    }
    return hash;
}

inline void annotate_genesis_episode_program_identity(
    vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id,
    const genesis_state& state) {
    using namespace vgmtooling::model;

    node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("Genesis program identity requires a physical voice episode");

    const attribute* family_item = find_genesis_part_attribute(*episode, "device_family");
    const attribute* instance_item = find_genesis_part_attribute(*episode, "instance");
    const attribute* channel_item = find_genesis_part_attribute(*episode, "physical_channel");
    if (family_item == nullptr || instance_item == nullptr || channel_item == nullptr)
        throw std::invalid_argument("Genesis physical episode is missing device identity attributes");

    const auto* family = std::get_if<std::string>(&family_item->value);
    const auto* instance = std::get_if<std::uint64_t>(&instance_item->value);
    const auto* channel = std::get_if<std::uint64_t>(&channel_item->value);
    if (family == nullptr || instance == nullptr || channel == nullptr)
        throw std::invalid_argument("Genesis physical episode device attributes have unexpected types");

    if (*family == "YM2612") {
        if (*instance > 1 || *channel >= state.ym2612(static_cast<std::size_t>(*instance)).channels.size())
            throw std::invalid_argument("Genesis YM2612 physical episode is out of range");
        const auto& voice = state.ym2612(static_cast<std::size_t>(*instance)).channels[
            static_cast<std::size_t>(*channel)];
        episode->attributes.push_back({
            "instrument_program_fingerprint",
            ym2612_program_fingerprint(voice),
            evidence_status::derived,
            1.0,
            "fnv1a64",
        });
        episode->attributes.push_back({
            "instrument_program_scope",
            std::string{"ym2612_pitch_route_invariant_channel_program"},
            evidence_status::derived,
            1.0,
            "",
        });
        return;
    }

    if (*family == "SN76489") {
        // The PSG tone channels do not have a patch object comparable to a
        // YM2612 four-operator program. Preserve only the synthesis class; do
        // not promote it to identity-bearing evidence.
        episode->attributes.push_back({
            "instrument_program_scope",
            std::string{"sn76489_tone_generator_class_only"},
            evidence_status::derived,
            1.0,
            "",
        });
    }
}

inline genesis_performance_append_result append_genesis_part_aware_record(
    vgmtooling::model::musical_execution_graph& graph,
    genesis_performance_graph_handle& handle,
    const command_trace_record& record) {
    auto result = append_genesis_performance_record(graph, handle, record);
    if (result.physical_voice_episode_id.has_value() &&
        result.performance_event_id.has_value()) {
        const auto* event = graph.find_node(*result.performance_event_id);
        const auto* event_kind = event == nullptr
            ? nullptr
            : find_genesis_part_attribute(*event, "event_kind");
        const auto* kind = event_kind == nullptr
            ? nullptr
            : std::get_if<std::string>(&event_kind->value);
        if (kind != nullptr && *kind == "pitched_activity_onset")
            annotate_genesis_episode_program_identity(
                graph,
                *result.physical_voice_episode_id,
                handle.execution.shadow);
    }
    return result;
}

inline const vgmtooling::model::node* genesis_episode_onset_event(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) noexcept {
    using namespace vgmtooling::model;
    const auto incoming = graph.edges_to(episode_id, edge_kind::realizes);
    for (const edge* relation : incoming) {
        const node* event = graph.find_node(relation->from);
        if (event == nullptr || event->kind != node_kind::musical_event)
            continue;
        const attribute* kind_item = find_genesis_part_attribute(*event, "event_kind");
        const auto* kind = kind_item == nullptr
            ? nullptr
            : std::get_if<std::string>(&kind_item->value);
        if (kind != nullptr && *kind == "pitched_activity_onset")
            return event;
    }
    return nullptr;
}

inline std::optional<double> genesis_relative_pitch_coordinate(
    const vgmtooling::model::node& onset) noexcept {
    const attribute* family_item = find_genesis_part_attribute(onset, "device_family");
    const attribute* pitch_item = find_genesis_part_attribute(onset, "device_pitch_code");
    if (family_item == nullptr || pitch_item == nullptr)
        return std::nullopt;
    const auto* family = std::get_if<std::string>(&family_item->value);
    const auto* pitch = std::get_if<std::uint64_t>(&pitch_item->value);
    if (family == nullptr || pitch == nullptr || *pitch == 0)
        return std::nullopt;

    if (*family == "YM2612") {
        const attribute* block_item = find_genesis_part_attribute(onset, "device_pitch_block");
        const auto* block = block_item == nullptr
            ? nullptr
            : std::get_if<std::uint64_t>(&block_item->value);
        if (block == nullptr || *block > 7 || *pitch > 0x7ff)
            return std::nullopt;
        return static_cast<double>(*pitch) * std::ldexp(1.0, static_cast<int>(*block));
    }

    if (*family == "SN76489") {
        if (*pitch > 0x3ff)
            return std::nullopt;
        return 1.0 / static_cast<double>(*pitch);
    }

    return std::nullopt;
}

struct genesis_part_continuity_policy {
    // VGM source ticks are compared only within the same trace. The caller
    // chooses a musically meaningful gap bound for the corpus under analysis.
    std::uint64_t max_gap_ticks = 0;
    double max_pitch_interval_octaves = 2.0;
};

inline vgmtooling::model::persistent_part_hypothesis infer_genesis_persistent_part(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id first_episode_id,
    vgmtooling::model::node_id second_episode_id,
    std::string source,
    genesis_part_continuity_policy policy) {
    using namespace vgmtooling::model;

    const node* first = graph.find_node(first_episode_id);
    const node* second = graph.find_node(second_episode_id);
    if (first == nullptr || second == nullptr ||
        first->kind != node_kind::voice_instance || second->kind != node_kind::voice_instance)
        throw std::invalid_argument("Genesis part inference requires two physical voice episodes");
    if (source.empty())
        throw std::invalid_argument("Genesis part inference requires a source label");
    if (!std::isfinite(policy.max_pitch_interval_octaves) ||
        policy.max_pitch_interval_octaves < 0.0)
        throw std::invalid_argument("Genesis part pitch interval bound must be finite and nonnegative");

    std::vector<persistent_part_evidence> evidence;
    double proposed = 0.30;

    const auto* first_family_item = find_genesis_part_attribute(*first, "device_family");
    const auto* second_family_item = find_genesis_part_attribute(*second, "device_family");
    const auto* first_instance_item = find_genesis_part_attribute(*first, "instance");
    const auto* second_instance_item = find_genesis_part_attribute(*second, "instance");
    const auto* first_channel_item = find_genesis_part_attribute(*first, "physical_channel");
    const auto* second_channel_item = find_genesis_part_attribute(*second, "physical_channel");

    const auto* first_family = first_family_item == nullptr
        ? nullptr : std::get_if<std::string>(&first_family_item->value);
    const auto* second_family = second_family_item == nullptr
        ? nullptr : std::get_if<std::string>(&second_family_item->value);
    const auto* first_instance = first_instance_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&first_instance_item->value);
    const auto* second_instance = second_instance_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&second_instance_item->value);
    const auto* first_channel = first_channel_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&first_channel_item->value);
    const auto* second_channel = second_channel_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&second_channel_item->value);

    if (first_family != nullptr && second_family != nullptr &&
        first_instance != nullptr && second_instance != nullptr &&
        first_channel != nullptr && second_channel != nullptr &&
        *first_family == *second_family && *first_instance == *second_instance &&
        *first_channel == *second_channel) {
        evidence.push_back({
            persistent_part_evidence_kind::physical_slot_continuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::derived,
            0.55,
            source,
            "successive bounded episodes occupy the same Genesis device channel; useful continuity cue but not musical identity by itself",
            {first_episode_id, second_episode_id},
        });
        proposed += 0.08;
    }

    const attribute* first_program = find_genesis_part_attribute(*first, "instrument_program_fingerprint");
    const attribute* second_program = find_genesis_part_attribute(*second, "instrument_program_fingerprint");
    const auto* first_program_id = first_program == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&first_program->value);
    const auto* second_program_id = second_program == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&second_program->value);
    if (first_program_id != nullptr && second_program_id != nullptr) {
        if (*first_program_id == *second_program_id) {
            evidence.push_back({
                persistent_part_evidence_kind::instrument_program_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                source,
                "episodes begin with the same pitch- and route-invariant YM2612 programmed instrument fingerprint",
                {first_episode_id, second_episode_id},
            });
            proposed += 0.31;
        } else {
            evidence.push_back({
                persistent_part_evidence_kind::identity_discontinuity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.62,
                source,
                "YM2612 programmed instrument fingerprint changes between episodes; a continuing part remains possible but needs stronger evidence",
                {first_episode_id, second_episode_id},
            });
            proposed -= 0.08;
        }
    }

    if (first->active.has_value() && first->active->end.has_value() &&
        second->active.has_value() &&
        first->active->end->domain == second->active->start.domain) {
        const std::int64_t gap = second->active->start.tick - first->active->end->tick;
        if (gap < 0) {
            evidence.push_back({
                persistent_part_evidence_kind::simultaneous_conflict,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.88,
                source,
                "candidate episodes overlap in the same source-time domain, conflicting with simple one-strand continuity",
                {first_episode_id, second_episode_id},
            });
            proposed -= 0.20;
        } else if (policy.max_gap_ticks > 0 &&
                   static_cast<std::uint64_t>(gap) <= policy.max_gap_ticks) {
            const double closeness = 1.0 -
                static_cast<double>(gap) / static_cast<double>(policy.max_gap_ticks);
            evidence.push_back({
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.70 + 0.20 * std::clamp(closeness, 0.0, 1.0),
                source,
                "second episode begins within the caller-established source-tick continuity window",
                {first_episode_id, second_episode_id},
            });
            proposed += 0.20;
        }
    }

    const node* first_onset = genesis_episode_onset_event(graph, first_episode_id);
    const node* second_onset = genesis_episode_onset_event(graph, second_episode_id);
    if (first_onset != nullptr && second_onset != nullptr) {
        const attribute* onset_family_a = find_genesis_part_attribute(*first_onset, "device_family");
        const attribute* onset_family_b = find_genesis_part_attribute(*second_onset, "device_family");
        const auto* family_a = onset_family_a == nullptr
            ? nullptr : std::get_if<std::string>(&onset_family_a->value);
        const auto* family_b = onset_family_b == nullptr
            ? nullptr : std::get_if<std::string>(&onset_family_b->value);
        const auto pitch_a = genesis_relative_pitch_coordinate(*first_onset);
        const auto pitch_b = genesis_relative_pitch_coordinate(*second_onset);
        if (family_a != nullptr && family_b != nullptr && *family_a == *family_b &&
            pitch_a.has_value() && pitch_b.has_value() && *pitch_a > 0.0 && *pitch_b > 0.0) {
            const double octaves = std::fabs(std::log2(*pitch_b / *pitch_a));
            if (octaves <= policy.max_pitch_interval_octaves) {
                const double interval_fit = policy.max_pitch_interval_octaves == 0.0
                    ? (octaves == 0.0 ? 1.0 : 0.0)
                    : 1.0 - octaves / policy.max_pitch_interval_octaves;
                evidence.push_back({
                    persistent_part_evidence_kind::pitch_trajectory_continuity,
                    persistent_part_evidence_origin::musical_analysis,
                    persistent_part_evidence_polarity::supports,
                    evidence_status::hypothesis,
                    0.58 + 0.18 * std::clamp(interval_fit, 0.0, 1.0),
                    source,
                    "source-relative onset pitch coordinates form a bounded interval compatible with a continuing line; this does not establish a note name",
                    {first_onset->id, second_onset->id},
                });
                proposed += 0.13;
            }
        }
    }

    // If no positive cue survived, preserve the model's invariant rather than
    // inventing a continuity hypothesis from absence of counterevidence.
    bool has_support = false;
    for (const auto& item : evidence)
        has_support = has_support || item.polarity == persistent_part_evidence_polarity::supports;
    if (!has_support)
        throw std::invalid_argument("Genesis episodes do not contain enough positive evidence for part continuity");

    proposed = std::clamp(proposed, 0.0, 0.94);
    return make_persistent_part_hypothesis(
        proposed,
        {first_episode_id, second_episode_id},
        std::move(evidence));
}

} // namespace gameaudio::vgm
