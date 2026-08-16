#pragma once

#include "phrase_boundary_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct part_phrase_boundary_hypothesis {
    node_id part_id = 0;
    phrase_boundary_hypothesis boundary;
};

struct phrase_boundary_consensus {
    time_coordinate representative{};
    time_span alignment_span{};
    double confidence = 0.0;
    double independent_part_ceiling = 0.0;
    std::vector<node_id> supporting_parts;
    std::vector<phrase_boundary_hypothesis> part_hypotheses;
    bool cross_part_grounded = false;
    bool authored_grounded = false;
};

constexpr double single_part_global_phrase_ceiling = 0.69;

inline bool compatible_phrase_boundary_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration;
}

inline phrase_boundary_consensus make_phrase_boundary_consensus(
    std::vector<part_phrase_boundary_hypothesis> hypotheses,
    std::int64_t alignment_tolerance_ticks = 0) {
    if (hypotheses.empty())
        throw std::invalid_argument("phrase-boundary consensus requires at least one part hypothesis");
    if (alignment_tolerance_ticks < 0)
        throw std::invalid_argument("phrase-boundary alignment tolerance must be nonnegative");

    const time_coordinate basis = hypotheses.front().boundary.boundary;
    std::set<node_id> parts;
    std::map<node_id, double> best_part_confidence;
    std::vector<std::int64_t> ticks;
    bool authored_grounded = false;

    for (const auto& item : hypotheses) {
        if (item.part_id == 0)
            throw std::invalid_argument("phrase-boundary consensus requires persistent-part ids");
        if (!compatible_phrase_boundary_time_basis(basis, item.boundary.boundary))
            throw std::invalid_argument("phrase-boundary consensus requires one compatible time basis");
        if (std::llabs(item.boundary.boundary.tick - basis.tick) > alignment_tolerance_ticks)
            throw std::invalid_argument("phrase-boundary hypotheses exceed the requested alignment tolerance");

        parts.insert(item.part_id);
        best_part_confidence[item.part_id] = std::max(
            best_part_confidence[item.part_id],
            item.boundary.confidence);
        ticks.push_back(item.boundary.boundary.tick);
        authored_grounded = authored_grounded || item.boundary.authored_grounded;
    }

    std::sort(ticks.begin(), ticks.end());
    const std::int64_t minimum_tick = ticks.front();
    const std::int64_t maximum_tick = ticks.back();
    const std::int64_t representative_tick =
        minimum_tick + (maximum_tick - minimum_tick) / 2;

    std::vector<double> strengths;
    strengths.reserve(best_part_confidence.size());
    for (const auto& item : best_part_confidence)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});
    const double independent_ceiling = strengths.size() >= 2
        ? strengths[1]
        : strengths.front();

    phrase_boundary_consensus result;
    result.representative = basis;
    result.representative.tick = representative_tick;
    result.alignment_span = time_span{
        time_coordinate{basis.domain, minimum_tick, basis.tick_rate, basis.loop_iteration},
        time_coordinate{basis.domain, maximum_tick, basis.tick_rate, basis.loop_iteration},
    };
    result.independent_part_ceiling = independent_ceiling;
    result.supporting_parts.assign(parts.begin(), parts.end());
    result.part_hypotheses.reserve(hypotheses.size());
    for (auto& item : hypotheses)
        result.part_hypotheses.push_back(std::move(item.boundary));
    result.cross_part_grounded = parts.size() >= 2;
    result.authored_grounded = authored_grounded;

    double confidence = independent_ceiling;
    if (!result.cross_part_grounded && !authored_grounded)
        confidence = std::min(confidence, single_part_global_phrase_ceiling);
    result.confidence = confidence;
    return result;
}

inline std::vector<phrase_boundary_consensus> group_phrase_boundaries_across_parts(
    std::vector<part_phrase_boundary_hypothesis> hypotheses,
    std::int64_t alignment_tolerance_ticks) {
    if (alignment_tolerance_ticks < 0)
        throw std::invalid_argument("phrase-boundary alignment tolerance must be nonnegative");
    if (hypotheses.empty())
        return {};

    std::sort(hypotheses.begin(), hypotheses.end(), [](const auto& first, const auto& second) {
        const auto& a = first.boundary.boundary;
        const auto& b = second.boundary.boundary;
        if (a.domain != b.domain)
            return static_cast<int>(a.domain) < static_cast<int>(b.domain);
        if (a.tick_rate != b.tick_rate)
            return a.tick_rate < b.tick_rate;
        if (a.loop_iteration != b.loop_iteration)
            return a.loop_iteration < b.loop_iteration;
        return a.tick < b.tick;
    });

    std::vector<phrase_boundary_consensus> groups;
    std::vector<part_phrase_boundary_hypothesis> current;
    time_coordinate anchor = hypotheses.front().boundary.boundary;

    for (auto& item : hypotheses) {
        const auto& coordinate = item.boundary.boundary;
        const bool compatible = compatible_phrase_boundary_time_basis(anchor, coordinate);
        const bool within = compatible &&
            std::llabs(coordinate.tick - anchor.tick) <= alignment_tolerance_ticks;
        if (!current.empty() && !within) {
            groups.push_back(make_phrase_boundary_consensus(
                std::move(current),
                alignment_tolerance_ticks));
            current.clear();
            anchor = coordinate;
        }
        if (current.empty())
            anchor = coordinate;
        current.push_back(std::move(item));
    }

    if (!current.empty()) {
        groups.push_back(make_phrase_boundary_consensus(
            std::move(current),
            alignment_tolerance_ticks));
    }
    return groups;
}

inline node_id add_phrase_boundary_consensus(
    musical_execution_graph& graph,
    const phrase_boundary_consensus& consensus) {
    for (node_id part_id : consensus.supporting_parts) {
        const node* part = graph.find_node(part_id);
        if (part == nullptr || part->kind != node_kind::part)
            throw std::invalid_argument("global phrase boundary references an unknown persistent part");
    }

    node boundary;
    boundary.kind = node_kind::musical_relation;
    boundary.layer = semantic_layer::musical_structure;
    boundary.flow = flow_kind::event;
    boundary.label = "cross-part phrase boundary hypothesis";
    boundary.active = consensus.alignment_span;
    boundary.attributes.push_back({
        "identity_scope",
        std::string{"global_phrase_boundary_hypothesis"},
        evidence_status::hypothesis,
        consensus.confidence,
        "",
    });
    boundary.attributes.push_back({
        "representative_tick",
        static_cast<std::int64_t>(consensus.representative.tick),
        evidence_status::derived,
        1.0,
        "ticks",
    });
    boundary.attributes.push_back({
        "supporting_part_count",
        static_cast<std::uint64_t>(consensus.supporting_parts.size()),
        evidence_status::derived,
        1.0,
        "parts",
    });
    boundary.attributes.push_back({
        "cross_part_grounded",
        consensus.cross_part_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    const node_id boundary_id = graph.add_node(std::move(boundary));

    for (node_id part_id : consensus.supporting_parts) {
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = part_id;
        support.to = boundary_id;
        support.attributes.push_back({
            "support_role",
            std::string{"persistent_part_phrase_boundary"},
            evidence_status::derived,
            1.0,
            "",
        });
        graph.add_edge(std::move(support));
    }
    return boundary_id;
}

} // namespace vgmtooling::model
