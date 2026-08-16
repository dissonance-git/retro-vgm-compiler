#pragma once

#include "spc_part_evidence.h"
#include "../../model/orchestration_transition_hypothesis.h"

#include <cmath>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace gameaudio::spc {

inline bool spc_orchestration_span_overlap(
    const vgmtooling::model::time_span& first,
    const vgmtooling::model::time_span& second) noexcept {
    using namespace vgmtooling::model;
    if (!first.end.has_value() || !second.end.has_value())
        return false;
    if (!part_role_same_time_basis(first.start, second.start) ||
        !part_role_same_time_basis(*first.end, *second.end)) {
        return false;
    }
    return first.start.tick < second.end->tick && second.start.tick < first.end->tick;
}

struct spc_part_orchestration_summary {
    std::vector<vgmtooling::model::node_id> episode_ids;
    std::set<vgmtooling::model::node_id> sample_version_ids;
    std::optional<double> register_center_log2_pitch_rate{};
    bool all_relevant_episodes_have_one_sample_version = true;
};

inline spc_part_orchestration_summary summarize_spc_part_orchestration(
    const vgmtooling::model::musical_execution_graph& graph,
    const vgmtooling::model::musical_part_role_hypothesis& role) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(role.part_id);
    if (part == nullptr || !is_persistent_musical_part_node(*part))
        throw std::invalid_argument("SPC orchestration summary requires a persistent musical part");

    spc_part_orchestration_summary result;
    std::vector<double> log_pitch_rates;

    for (const edge* relation : graph.edges_to(role.part_id, edge_kind::groups_into)) {
        const node* episode = graph.find_node(relation->from);
        if (episode == nullptr || episode->kind != node_kind::voice_instance ||
            !episode->active.has_value() ||
            !spc_orchestration_span_overlap(*episode->active, role.active)) {
            continue;
        }

        result.episode_ids.push_back(episode->id);
        const auto samples = spc_episode_sample_versions(graph, episode->id);
        if (samples.size() != 1) {
            result.all_relevant_episodes_have_one_sample_version = false;
        } else {
            result.sample_version_ids.insert(*samples.begin());
        }

        const node* pitch_event = spc_episode_pitch_observation(graph, episode->id);
        if (pitch_event != nullptr) {
            const attribute* pitch_item = find_spc_performance_attribute(*pitch_event, "pitch_rate");
            const auto* pitch = pitch_item == nullptr
                ? nullptr
                : std::get_if<std::uint64_t>(&pitch_item->value);
            if (pitch != nullptr && *pitch > 0)
                log_pitch_rates.push_back(std::log2(static_cast<double>(*pitch)));
        }
    }

    // S-DSP pitch rate is a register coordinate only when the same exact BRR
    // source version persists across the span. Unknown/different sample roots
    // make cross-sample register comparison musically unsafe.
    if (result.all_relevant_episodes_have_one_sample_version &&
        result.sample_version_ids.size() == 1 && !log_pitch_rates.empty()) {
        double sum = 0.0;
        for (double value : log_pitch_rates)
            sum += value;
        result.register_center_log2_pitch_rate =
            sum / static_cast<double>(log_pitch_rates.size());
    }
    return result;
}

inline vgmtooling::model::part_orchestration_state make_spc_part_orchestration_state(
    const vgmtooling::model::musical_execution_graph& graph,
    const vgmtooling::model::musical_part_role_hypothesis& role,
    std::string source) {
    using namespace vgmtooling::model;
    if (source.empty())
        throw std::invalid_argument("SPC orchestration state requires a source");

    const auto summary = summarize_spc_part_orchestration(graph, role);
    std::optional<orchestration_realization> realization;
    if (summary.all_relevant_episodes_have_one_sample_version &&
        summary.sample_version_ids.size() == 1) {
        realization = orchestration_realization{
            "spc_brr_sample_version_node",
            std::to_string(*summary.sample_version_ids.begin()),
            evidence_status::derived,
            1.0,
            source,
        };
    }

    return make_part_orchestration_state(
        role,
        std::move(realization),
        summary.register_center_log2_pitch_rate,
        summary.register_center_log2_pitch_rate.has_value()
            ? std::string{"spc_same_sample_log2_pitch_rate"}
            : std::string{});
}

} // namespace gameaudio::spc
