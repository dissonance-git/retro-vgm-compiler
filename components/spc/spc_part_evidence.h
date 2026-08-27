#pragma once

#include "spc_performance_adapter.h"
#include "../../model/persistent_part_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::spc {

inline bool spc_episode_allows_part_successor(
    const vgmtooling::model::node& episode) noexcept {
    const auto* complete_item = find_spc_performance_attribute(
        episode,
        "termination_boundary_complete");
    const auto* complete = complete_item == nullptr
        ? nullptr : std::get_if<bool>(&complete_item->value);
    if (complete != nullptr && !*complete)
        return false;

    const auto* reason_item = find_spc_performance_attribute(
        episode,
        "termination_reason");
    const auto* reason = reason_item == nullptr
        ? nullptr : std::get_if<std::string>(&reason_item->value);
    return reason == nullptr ||
        (*reason != "semantic_continuation_lost" && *reason != "execution_reset");
}

inline std::vector<const vgmtooling::model::node*> spc_episode_runtime_events(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) {
    using namespace vgmtooling::model;

    std::vector<const node*> result;
    std::set<node_id> seen;
    for (edge_kind kind : {edge_kind::causes, edge_kind::contributes_to}) {
        const auto incoming = graph.edges_to(episode_id, kind);
        for (const edge* relation : incoming) {
            const node* event = graph.find_node(relation->from);
            if (event != nullptr && event->kind == node_kind::trace_event &&
                seen.insert(event->id).second)
                result.push_back(event);
        }
    }
    return result;
}

inline std::set<vgmtooling::model::node_id> spc_episode_sample_versions(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) {
    using namespace vgmtooling::model;

    std::set<node_id> samples;
    for (const node* event : spc_episode_runtime_events(graph, episode_id)) {
        const auto references = graph.edges_from(event->id, edge_kind::references);
        for (const edge* relation : references) {
            const node* sample = graph.find_node(relation->to);
            if (sample != nullptr && sample->kind == node_kind::sample_buffer)
                samples.insert(sample->id);
        }
    }
    return samples;
}

inline const vgmtooling::model::node* spc_episode_pitch_observation(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) noexcept {
    for (const auto* event : spc_episode_runtime_events(graph, episode_id)) {
        const auto* kind_item = find_spc_performance_attribute(*event, "event_kind");
        const auto* kind = kind_item == nullptr
            ? nullptr : std::get_if<std::string>(&kind_item->value);
        const auto* pitch_item = find_spc_performance_attribute(*event, "pitch_rate");
        const auto* pitch = pitch_item == nullptr
            ? nullptr : std::get_if<std::uint64_t>(&pitch_item->value);
        if (kind != nullptr && pitch != nullptr && *pitch > 0 &&
            (*kind == "key_on_accepted" || *kind == "sample_phase_started"))
            return event;
    }
    return nullptr;
}

struct spc_part_continuity_policy {
    double max_gap_seconds = 0.50;
    double max_pitch_interval_octaves = 2.0;
};

// Same BRR source, local timing, and source-relative pitch can all remain useful
// cross-voice evidence, but they do not establish that a musical part actually
// handed off between two S-DSP slots. Preserve that hypothesis while keeping it
// below strong trajectory-link promotion until an independent handoff witness is
// modeled and arbitrated explicitly.
constexpr double spc_unarbitrated_cross_voice_confidence_ceiling = 0.74;

inline vgmtooling::model::persistent_part_hypothesis infer_spc_persistent_part(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id first_episode_id,
    vgmtooling::model::node_id second_episode_id,
    std::string source,
    spc_part_continuity_policy policy = {}) {
    using namespace vgmtooling::model;

    const node* first = graph.find_node(first_episode_id);
    const node* second = graph.find_node(second_episode_id);
    if (first == nullptr || second == nullptr ||
        first->kind != node_kind::voice_instance || second->kind != node_kind::voice_instance)
        throw std::invalid_argument("SPC part inference requires two physical voice episodes");
    if (source.empty())
        throw std::invalid_argument("SPC part inference requires a source label");
    if (!std::isfinite(policy.max_gap_seconds) || policy.max_gap_seconds < 0.0 ||
        !std::isfinite(policy.max_pitch_interval_octaves) ||
        policy.max_pitch_interval_octaves < 0.0)
        throw std::invalid_argument("SPC part continuity policy must be finite and nonnegative");

    std::vector<persistent_part_evidence> evidence;
    double proposed = 0.30;

    const attribute* first_voice_item = find_spc_performance_attribute(*first, "physical_voice");
    const attribute* second_voice_item = find_spc_performance_attribute(*second, "physical_voice");
    const auto* first_voice = first_voice_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&first_voice_item->value);
    const auto* second_voice = second_voice_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&second_voice_item->value);
    const bool explicit_cross_voice = first_voice != nullptr && second_voice != nullptr &&
        *first_voice != *second_voice;
    if (first_voice != nullptr && second_voice != nullptr && *first_voice == *second_voice) {
        evidence.push_back({
            persistent_part_evidence_kind::physical_slot_continuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::exact,
            0.55,
            source,
            "successive episodes occupy the same S-DSP physical voice; useful continuity cue but not musical identity by itself",
            {first_episode_id, second_episode_id},
        });
        proposed += 0.08;
    }

    const auto first_samples = spc_episode_sample_versions(graph, first_episode_id);
    const auto second_samples = spc_episode_sample_versions(graph, second_episode_id);
    const bool first_sample_exact = first_samples.size() == 1;
    const bool second_sample_exact = second_samples.size() == 1;
    const bool same_sample = first_sample_exact && second_sample_exact &&
        *first_samples.begin() == *second_samples.begin();

    if (same_sample) {
        evidence.push_back({
            persistent_part_evidence_kind::source_identity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::derived,
            0.97,
            source,
            "both episodes reference the same event-time BRR RAM-version object; source identity survives SRCN reuse and later RAM writes",
            {*first_samples.begin(), first_episode_id, second_episode_id},
        });
        proposed += 0.31;
    } else if (first_sample_exact && second_sample_exact) {
        evidence.push_back({
            persistent_part_evidence_kind::identity_discontinuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::counters,
            evidence_status::derived,
            0.60,
            source,
            "episodes reference different event-time BRR sample-version objects; a continuing musical part is possible but requires stronger evidence",
            {*first_samples.begin(), *second_samples.begin(), first_episode_id, second_episode_id},
        });
        proposed -= 0.08;
    }

    if (first->active.has_value() && first->active->end.has_value() &&
        second->active.has_value() &&
        first->active->end->domain == second->active->start.domain &&
        first->active->end->tick_rate > 0 &&
        first->active->end->tick_rate == second->active->start.tick_rate) {
        const std::int64_t gap_ticks = second->active->start.tick - first->active->end->tick;
        if (gap_ticks < 0) {
            evidence.push_back({
                persistent_part_evidence_kind::simultaneous_conflict,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.90,
                source,
                "candidate S-DSP episodes overlap in device time, conflicting with simple one-strand continuity",
                {first_episode_id, second_episode_id},
            });
            proposed -= 0.20;
        } else if (policy.max_gap_seconds > 0.0) {
            const double gap_seconds = static_cast<double>(gap_ticks) /
                static_cast<double>(first->active->end->tick_rate);
            if (gap_seconds <= policy.max_gap_seconds) {
                const double closeness = 1.0 - gap_seconds / policy.max_gap_seconds;
                evidence.push_back({
                    persistent_part_evidence_kind::temporal_adjacency,
                    persistent_part_evidence_origin::musical_analysis,
                    persistent_part_evidence_polarity::supports,
                    evidence_status::derived,
                    0.72 + 0.18 * std::clamp(closeness, 0.0, 1.0),
                    source,
                    "second S-DSP episode begins within the device-time continuity window",
                    {first_episode_id, second_episode_id},
                });
                proposed += 0.20;
            }
        }
    }

    const node* first_pitch_event = spc_episode_pitch_observation(graph, first_episode_id);
    const node* second_pitch_event = spc_episode_pitch_observation(graph, second_episode_id);
    if (same_sample && first_pitch_event != nullptr && second_pitch_event != nullptr) {
        const attribute* first_pitch_item = find_spc_performance_attribute(*first_pitch_event, "pitch_rate");
        const attribute* second_pitch_item = find_spc_performance_attribute(*second_pitch_event, "pitch_rate");
        const auto* first_pitch = first_pitch_item == nullptr
            ? nullptr : std::get_if<std::uint64_t>(&first_pitch_item->value);
        const auto* second_pitch = second_pitch_item == nullptr
            ? nullptr : std::get_if<std::uint64_t>(&second_pitch_item->value);
        if (first_pitch != nullptr && second_pitch != nullptr && *first_pitch > 0 && *second_pitch > 0) {
            // With the same exact BRR source, unknown root tuning cancels out:
            // the ratio of S-DSP pitch rates is a source-relative interval.
            const double octaves = std::fabs(std::log2(
                static_cast<double>(*second_pitch) / static_cast<double>(*first_pitch)));
            if (octaves <= policy.max_pitch_interval_octaves) {
                const double interval_fit = policy.max_pitch_interval_octaves == 0.0
                    ? (octaves == 0.0 ? 1.0 : 0.0)
                    : 1.0 - octaves / policy.max_pitch_interval_octaves;
                evidence.push_back({
                    persistent_part_evidence_kind::pitch_trajectory_continuity,
                    persistent_part_evidence_origin::musical_analysis,
                    persistent_part_evidence_polarity::supports,
                    evidence_status::derived,
                    0.62 + 0.18 * std::clamp(interval_fit, 0.0, 1.0),
                    source,
                    "same event-time BRR source makes the pitch-rate ratio a root-tuning-independent relative interval compatible with a continuing line",
                    {first_pitch_event->id, second_pitch_event->id},
                });
                proposed += 0.13;
            }
        }
    }

    bool has_support = false;
    for (const auto& item : evidence)
        has_support = has_support || item.polarity == persistent_part_evidence_polarity::supports;
    if (!has_support)
        throw std::invalid_argument("SPC episodes do not contain enough positive evidence for part continuity");

    // Runtime-only identity remains a strong hypothesis, not certainty.
    proposed = std::clamp(proposed, 0.0, 0.88);
    auto hypothesis = make_persistent_part_hypothesis(
        proposed,
        {first_episode_id, second_episode_id},
        std::move(evidence));

    if (explicit_cross_voice) {
        hypothesis.confidence = std::min(
            hypothesis.confidence,
            spc_unarbitrated_cross_voice_confidence_ceiling);
    }
    return hypothesis;
}

} // namespace gameaudio::spc
