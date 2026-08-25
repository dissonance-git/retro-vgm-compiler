#pragma once

#include "spc_part_evidence.h"
#include "spc_part_motif_adapter.h"
#include "../../model/persistent_part_trajectory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace gameaudio::spc {

// Label-blind by construction. This layer accepts only runtime musical evidence;
// catalog metadata, external tags, GD3, ID666, game titles, and author names have
// no input surface here. Metadata may be joined only after these features exist.
struct spc_label_blind_corpus_features {
    std::size_t voice_episode_count = 0;
    std::size_t eligible_episode_count = 0;
    std::size_t candidate_transition_count = 0;
    std::size_t strong_transition_count = 0;
    std::size_t rejected_transition_count = 0;
    std::size_t continuity_barrier_count = 0;
    std::size_t emitted_part_count = 0;
    std::vector<vgmtooling::model::part_motif_profile> part_profiles;
};

namespace detail {

struct spc_slot_time_basis {
    std::uint64_t physical_voice = 0;
    vgmtooling::model::time_domain domain = vgmtooling::model::time_domain::device;
    std::uint64_t tick_rate = 0;
    std::int64_t loop_iteration = 0;

    friend bool operator<(const spc_slot_time_basis& lhs, const spc_slot_time_basis& rhs) noexcept {
        return std::tie(lhs.physical_voice, lhs.domain, lhs.tick_rate, lhs.loop_iteration) <
               std::tie(rhs.physical_voice, rhs.domain, rhs.tick_rate, rhs.loop_iteration);
    }
};

inline const std::uint64_t* spc_episode_physical_voice(
    const vgmtooling::model::node& episode) noexcept {
    const auto* item = find_spc_performance_attribute(episode, "physical_voice");
    return item == nullptr ? nullptr : std::get_if<std::uint64_t>(&item->value);
}

inline void emit_spc_label_blind_trajectory(
    vgmtooling::model::musical_execution_graph& graph,
    std::vector<vgmtooling::model::persistent_part_hypothesis>& transitions,
    spc_label_blind_corpus_features& result) {
    using namespace vgmtooling::model;

    // A motif profile requires at least three gesture observations, therefore a
    // trajectory needs at least two pairwise links before it can become a corpus
    // feature. Short fragments remain counted as evidence but are not promoted.
    if (transitions.size() < 2) {
        transitions.clear();
        return;
    }

    const auto trajectory = make_persistent_part_trajectory(transitions);
    const node_id part_id = add_persistent_part_trajectory(graph, trajectory);
    ++result.emitted_part_count;

    const auto profile = make_spc_part_motif_profile(graph, part_id);
    if (profile.has_value())
        result.part_profiles.push_back(*profile);

    transitions.clear();
}

} // namespace detail

// Conservative first-pass recovery for corpus experiments.
//
// Episodes are partitioned by physical S-DSP voice AND exact local device-time
// basis, sorted by onset, and only adjacent episodes are considered. This avoids
// inventing cross-slot handoffs or cross-loop continuity. Explicit runtime loss
// and reset boundaries are hard barriers: later similarity cannot erase an
// observation gap. A future wider search may add cross-slot hypotheses explicitly,
// but this pass intentionally prefers false negatives over false musical identity.
//
// `runtime_source` is provenance for the inference itself, not a catalog label.
// It should identify the runtime capture/trace source without encoding authorship.
inline spc_label_blind_corpus_features extract_spc_label_blind_corpus_features(
    vgmtooling::model::musical_execution_graph& graph,
    std::string runtime_source,
    spc_part_continuity_policy policy = {}) {
    using namespace vgmtooling::model;

    if (runtime_source.empty())
        throw std::invalid_argument("SPC label-blind corpus extraction requires runtime provenance");
    if (!std::isfinite(policy.max_gap_seconds) || policy.max_gap_seconds < 0.0 ||
        !std::isfinite(policy.max_pitch_interval_octaves) ||
        policy.max_pitch_interval_octaves < 0.0)
        throw std::invalid_argument("SPC label-blind continuity policy must be finite and nonnegative");

    spc_label_blind_corpus_features result;
    const auto episodes = graph.nodes_of_kind(node_kind::voice_instance);
    result.voice_episode_count = episodes.size();

    std::map<detail::spc_slot_time_basis, std::vector<node_id>> buckets;
    for (const node* episode : episodes) {
        if (episode == nullptr || !episode->active.has_value())
            continue;
        const auto* voice = detail::spc_episode_physical_voice(*episode);
        if (voice == nullptr)
            continue;
        const auto& start = episode->active->start;
        if (start.domain != time_domain::device || start.tick_rate == 0)
            continue;

        buckets[{
            *voice,
            start.domain,
            start.tick_rate,
            start.loop_iteration,
        }].push_back(episode->id);
        ++result.eligible_episode_count;
    }

    for (auto& entry : buckets) {
        auto& ids = entry.second;
        std::sort(ids.begin(), ids.end(), [&graph](node_id lhs, node_id rhs) {
            const node* first = graph.find_node(lhs);
            const node* second = graph.find_node(rhs);
            if (first == nullptr || second == nullptr ||
                !first->active.has_value() || !second->active.has_value())
                return lhs < rhs;
            if (first->active->start.tick != second->active->start.tick)
                return first->active->start.tick < second->active->start.tick;
            return lhs < rhs;
        });

        std::vector<persistent_part_hypothesis> trajectory_links;
        for (std::size_t index = 1; index < ids.size(); ++index) {
            const node* previous = graph.find_node(ids[index - 1]);
            if (previous != nullptr && !spc_episode_allows_part_successor(*previous)) {
                ++result.continuity_barrier_count;
                detail::emit_spc_label_blind_trajectory(graph, trajectory_links, result);
                continue;
            }

            ++result.candidate_transition_count;
            try {
                auto hypothesis = infer_spc_persistent_part(
                    graph,
                    ids[index - 1],
                    ids[index],
                    runtime_source,
                    policy);

                if (strong_persistent_part_transition(hypothesis)) {
                    ++result.strong_transition_count;
                    trajectory_links.push_back(std::move(hypothesis));
                    continue;
                }
            } catch (const std::invalid_argument&) {
                // Lack of positive continuity evidence is a normal corpus outcome.
                // Structural and policy validity are checked before this point.
            }

            ++result.rejected_transition_count;
            detail::emit_spc_label_blind_trajectory(graph, trajectory_links, result);
        }

        detail::emit_spc_label_blind_trajectory(graph, trajectory_links, result);
    }

    return result;
}

} // namespace gameaudio::spc
