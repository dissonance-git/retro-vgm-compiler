#pragma once

#include "persistent_part_hypothesis.h"

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vgmtooling::model {

constexpr double persistent_part_trajectory_link_threshold = 0.75;

struct persistent_part_trajectory {
    std::vector<node_id> subject_nodes;
    std::vector<persistent_part_hypothesis> transitions;
    double confidence = 0.0;
};

struct persistent_part_successor_resolution {
    std::optional<node_id> successor{};
    bool ambiguous = false;
    std::vector<std::size_t> strong_transition_indices;
};

inline std::optional<node_id> persistent_part_pair_other_subject(
    const persistent_part_hypothesis& hypothesis,
    node_id subject) noexcept {
    if (hypothesis.subject_nodes.size() != 2)
        return std::nullopt;
    if (hypothesis.subject_nodes[0] == subject && hypothesis.subject_nodes[1] != subject)
        return hypothesis.subject_nodes[1];
    if (hypothesis.subject_nodes[1] == subject && hypothesis.subject_nodes[0] != subject)
        return hypothesis.subject_nodes[0];
    return std::nullopt;
}

inline bool strong_persistent_part_transition(
    const persistent_part_hypothesis& hypothesis,
    double threshold = persistent_part_trajectory_link_threshold) noexcept {
    return hypothesis.subject_nodes.size() == 2 &&
           !hypothesis.strong_conflict_present &&
           hypothesis.confidence >= threshold;
}

inline persistent_part_successor_resolution resolve_unique_persistent_part_successor(
    node_id tail,
    const std::vector<persistent_part_hypothesis>& candidates,
    const std::set<node_id>& already_used = {},
    double threshold = persistent_part_trajectory_link_threshold) {
    persistent_part_successor_resolution result;
    std::set<node_id> distinct_successors;

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const auto& candidate = candidates[index];
        if (!strong_persistent_part_transition(candidate, threshold))
            continue;
        const auto other = persistent_part_pair_other_subject(candidate, tail);
        if (!other.has_value() || already_used.count(*other) != 0)
            continue;
        result.strong_transition_indices.push_back(index);
        distinct_successors.insert(*other);
    }

    if (distinct_successors.size() == 1) {
        result.successor = *distinct_successors.begin();
        return result;
    }
    if (distinct_successors.size() > 1)
        result.ambiguous = true;
    return result;
}

inline persistent_part_trajectory make_persistent_part_trajectory(
    std::vector<persistent_part_hypothesis> transitions,
    double threshold = persistent_part_trajectory_link_threshold) {
    if (transitions.empty())
        throw std::invalid_argument("persistent-part trajectory requires at least one transition");
    if (threshold < 0.0 || threshold > 1.0)
        throw std::invalid_argument("persistent-part trajectory threshold must be in [0, 1]");

    for (const auto& transition : transitions) {
        if (transition.subject_nodes.size() != 2)
            throw std::invalid_argument("persistent-part trajectory transitions must be pairwise");
        if (transition.strong_conflict_present)
            throw std::invalid_argument("persistent-part trajectory cannot include a strongly conflicted transition");
        if (transition.confidence < threshold)
            throw std::invalid_argument("persistent-part trajectory cannot include a weak transition");
    }

    persistent_part_trajectory result;
    result.transitions = std::move(transitions);
    result.subject_nodes = result.transitions.front().subject_nodes;
    result.confidence = result.transitions.front().confidence;

    std::set<node_id> used(result.subject_nodes.begin(), result.subject_nodes.end());
    if (used.size() != 2)
        throw std::invalid_argument("persistent-part trajectory cannot begin with a self-link");

    for (std::size_t index = 1; index < result.transitions.size(); ++index) {
        const auto& transition = result.transitions[index];
        const node_id tail = result.subject_nodes.back();
        const auto other = persistent_part_pair_other_subject(transition, tail);
        if (!other.has_value())
            throw std::invalid_argument("persistent-part trajectory transition is not contiguous with the current tail");
        if (used.count(*other) != 0)
            throw std::invalid_argument("persistent-part trajectory cannot silently close a loop or reuse a prior subject");

        result.subject_nodes.push_back(*other);
        used.insert(*other);
        result.confidence = std::min(result.confidence, transition.confidence);
    }

    return result;
}

inline persistent_part_hypothesis persistent_part_hypothesis_from_trajectory(
    const persistent_part_trajectory& trajectory) {
    if (trajectory.subject_nodes.size() < 2 || trajectory.transitions.empty())
        throw std::invalid_argument("persistent-part trajectory is empty");

    std::vector<persistent_part_evidence> evidence;
    for (const auto& transition : trajectory.transitions) {
        evidence.insert(
            evidence.end(),
            transition.evidence.begin(),
            transition.evidence.end());
    }

    return make_persistent_part_hypothesis(
        trajectory.confidence,
        trajectory.subject_nodes,
        std::move(evidence));
}

inline node_id add_persistent_part_trajectory(
    musical_execution_graph& graph,
    const persistent_part_trajectory& trajectory) {
    return add_persistent_part_hypothesis(
        graph,
        persistent_part_hypothesis_from_trajectory(trajectory));
}

} // namespace vgmtooling::model
