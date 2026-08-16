#pragma once

#include "diatonic_key_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class tonal_region_topology : std::uint8_t {
    sequential = 0,
    target_nested_in_source,
    source_nested_in_target,
    overlapping,
};

enum class tonal_region_relation_kind : std::uint8_t {
    unresolved = 0,
    retained_center,
    contrasting_center,
    tonicization_candidate,
    modulation_candidate,
    return_candidate,
};

enum class tonal_region_relation_evidence_kind : std::uint8_t {
    phrase_partition = 0,
    structural_arrival,
    center_persistence,
    structural_pitch_collection,
    return_reentry,
    form_recurrence,
    authored_annotation,
    contradiction,
};

enum class tonal_region_relation_evidence_origin : std::uint8_t {
    phrase_structure = 0,
    harmony,
    pitch_collection,
    form,
    authored_source,
    external_annotation,
};

struct tonal_region_relation_evidence {
    tonal_region_relation_evidence_kind kind = tonal_region_relation_evidence_kind::phrase_partition;
    tonal_region_relation_evidence_origin origin = tonal_region_relation_evidence_origin::phrase_structure;
    double confidence = 0.0;
    std::string dependency_group;
    node_id source_node = 0;
    bool supports_relation = true;
    evidence_status status = evidence_status::hypothesis;
    std::string source;
};

struct tonal_region_relation_hypothesis {
    tonal_region_relation_kind kind = tonal_region_relation_kind::unresolved;
    tonal_region_topology topology = tonal_region_topology::overlapping;
    time_span source_region{};
    time_span target_region{};
    double source_center_octave_class = 0.0;
    double target_center_octave_class = 0.0;
    double center_distance_octaves = 0.0;
    bool centers_equivalent = false;
    bool target_center_cross_origin_grounded = false;
    bool target_key_class_resolved = false;
    bool return_reference_supplied = false;
    bool target_matches_return_reference = false;
    std::size_t independent_support_groups = 0;
    std::size_t independent_support_origins = 0;
    double independent_support_ceiling = 0.0;
    bool strong_counterevidence = false;
    bool modulation_established = false;
    bool tonicization_established = false;
    double confidence = 0.0;
    std::vector<tonal_region_relation_evidence> supporting_evidence;
    std::vector<tonal_region_relation_evidence> counterevidence;
};

constexpr double tonal_region_center_equivalence_tolerance_octaves = 35.0 / 1200.0;
constexpr double tonal_region_contrast_ceiling = 0.60;
constexpr double tonal_region_retained_center_ceiling = 0.82;
constexpr double tonal_region_tonicization_candidate_ceiling = 0.74;
constexpr double tonal_region_modulation_candidate_ceiling = 0.82;
constexpr double tonal_region_return_candidate_ceiling = 0.82;
constexpr double tonal_region_strong_conflict_ceiling = 0.49;
constexpr double tonal_region_strong_counterevidence_threshold = 0.70;
constexpr double tonal_region_min_grounded_center_confidence = 0.69;

inline const char* to_string(tonal_region_topology topology) noexcept {
    switch (topology) {
    case tonal_region_topology::sequential: return "sequential";
    case tonal_region_topology::target_nested_in_source: return "target_nested_in_source";
    case tonal_region_topology::source_nested_in_target: return "source_nested_in_target";
    case tonal_region_topology::overlapping: return "overlapping";
    }
    return "unknown";
}

inline const char* to_string(tonal_region_relation_kind kind) noexcept {
    switch (kind) {
    case tonal_region_relation_kind::unresolved: return "unresolved";
    case tonal_region_relation_kind::retained_center: return "retained_center";
    case tonal_region_relation_kind::contrasting_center: return "contrasting_center";
    case tonal_region_relation_kind::tonicization_candidate: return "tonicization_candidate";
    case tonal_region_relation_kind::modulation_candidate: return "modulation_candidate";
    case tonal_region_relation_kind::return_candidate: return "return_candidate";
    }
    return "unknown";
}

inline const char* to_string(tonal_region_relation_evidence_kind kind) noexcept {
    switch (kind) {
    case tonal_region_relation_evidence_kind::phrase_partition: return "phrase_partition";
    case tonal_region_relation_evidence_kind::structural_arrival: return "structural_arrival";
    case tonal_region_relation_evidence_kind::center_persistence: return "center_persistence";
    case tonal_region_relation_evidence_kind::structural_pitch_collection: return "structural_pitch_collection";
    case tonal_region_relation_evidence_kind::return_reentry: return "return_reentry";
    case tonal_region_relation_evidence_kind::form_recurrence: return "form_recurrence";
    case tonal_region_relation_evidence_kind::authored_annotation: return "authored_annotation";
    case tonal_region_relation_evidence_kind::contradiction: return "contradiction";
    }
    return "unknown";
}

inline const char* to_string(tonal_region_relation_evidence_origin origin) noexcept {
    switch (origin) {
    case tonal_region_relation_evidence_origin::phrase_structure: return "phrase_structure";
    case tonal_region_relation_evidence_origin::harmony: return "harmony";
    case tonal_region_relation_evidence_origin::pitch_collection: return "pitch_collection";
    case tonal_region_relation_evidence_origin::form: return "form";
    case tonal_region_relation_evidence_origin::authored_source: return "authored_source";
    case tonal_region_relation_evidence_origin::external_annotation: return "external_annotation";
    }
    return "unknown";
}

inline bool compatible_tonal_region_coordinate_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration;
}

inline bool compatible_tonal_region_time_basis(
    const time_span& first,
    const time_span& second) noexcept {
    return compatible_tonal_region_coordinate_basis(first.start, second.start);
}

inline bool finite_valid_tonal_region(const time_span& region) noexcept {
    return region.end.has_value() &&
           compatible_tonal_region_coordinate_basis(region.start, *region.end) &&
           region.end->tick > region.start.tick;
}

inline tonal_region_topology infer_tonal_region_topology(
    const time_span& source,
    const time_span& target) {
    if (!finite_valid_tonal_region(source) || !finite_valid_tonal_region(target))
        throw std::invalid_argument("tonal-region relation requires finite valid regions");
    if (!compatible_tonal_region_time_basis(source, target))
        throw std::invalid_argument("tonal-region relation requires one compatible time basis");

    if (source.end->tick <= target.start.tick)
        return tonal_region_topology::sequential;
    if (time_span_contains_span(source, target))
        return tonal_region_topology::target_nested_in_source;
    if (time_span_contains_span(target, source))
        return tonal_region_topology::source_nested_in_target;
    return tonal_region_topology::overlapping;
}

inline std::string tonal_region_dependency_key(const tonal_region_relation_evidence& evidence) {
    if (!evidence.dependency_group.empty())
        return evidence.dependency_group;
    return std::string{"origin:"} + to_string(evidence.origin);
}

inline bool has_relation_evidence_kind(
    const std::vector<tonal_region_relation_evidence>& evidence,
    tonal_region_relation_evidence_kind kind,
    double minimum_confidence = 0.0) noexcept {
    return std::any_of(evidence.begin(), evidence.end(), [&](const auto& item) {
        return item.supports_relation && item.kind == kind && item.confidence >= minimum_confidence;
    });
}

inline tonal_region_relation_hypothesis infer_tonal_region_relation(
    const tonal_center_hypothesis& source_center,
    const tonal_center_hypothesis& target_center,
    std::vector<tonal_region_relation_evidence> evidence,
    const std::optional<tonal_key_class_hypothesis>& target_key = std::nullopt,
    const std::optional<tonal_center_hypothesis>& return_reference = std::nullopt,
    double center_equivalence_tolerance_octaves = tonal_region_center_equivalence_tolerance_octaves) {
    if (!std::isfinite(center_equivalence_tolerance_octaves) ||
        center_equivalence_tolerance_octaves <= 0.0 ||
        center_equivalence_tolerance_octaves >= 0.5) {
        throw std::invalid_argument("tonal-region center equivalence tolerance is invalid");
    }

    tonal_region_relation_hypothesis result;
    result.topology = infer_tonal_region_topology(source_center.region, target_center.region);
    result.source_region = source_center.region;
    result.target_region = target_center.region;
    result.source_center_octave_class = normalize_octave_class(source_center.center_octave_class);
    result.target_center_octave_class = normalize_octave_class(target_center.center_octave_class);
    result.center_distance_octaves = circular_octave_class_distance(
        result.source_center_octave_class,
        result.target_center_octave_class);
    result.centers_equivalent = result.center_distance_octaves <= center_equivalence_tolerance_octaves;
    result.target_center_cross_origin_grounded = target_center.cross_origin_grounded;

    if (target_key.has_value()) {
        const auto& key = *target_key;
        if (!time_span_contains_span(target_center.region, key.region))
            throw std::invalid_argument("target key-class region must lie inside target tonal-center region");
        if (key.key_class_resolved &&
            circular_octave_class_distance(
                key.center_octave_class,
                target_center.center_octave_class) > center_equivalence_tolerance_octaves) {
            throw std::invalid_argument("resolved target key class disagrees with target tonal center");
        }
        result.target_key_class_resolved = key.key_class_resolved;
    }

    result.return_reference_supplied = return_reference.has_value();
    if (return_reference.has_value()) {
        if (!finite_valid_tonal_region(return_reference->region) ||
            !compatible_tonal_region_time_basis(return_reference->region, source_center.region) ||
            return_reference->region.end->tick > source_center.region.start.tick) {
            throw std::invalid_argument("return reference must be an earlier compatible tonal region");
        }
        result.target_matches_return_reference =
            circular_octave_class_distance(
                target_center.center_octave_class,
                return_reference->center_octave_class) <= center_equivalence_tolerance_octaves;
    }

    std::map<std::string, double> best_support_by_dependency;
    std::map<std::string, tonal_region_relation_evidence_origin> origin_by_dependency;
    for (auto& item : evidence) {
        if (!std::isfinite(item.confidence) || item.confidence < 0.0 || item.confidence > 1.0)
            throw std::invalid_argument("tonal-region relation evidence confidence must lie in [0, 1]");
        if (!item.supports_relation || item.kind == tonal_region_relation_evidence_kind::contradiction) {
            result.counterevidence.push_back(item);
            if (item.confidence >= tonal_region_strong_counterevidence_threshold)
                result.strong_counterevidence = true;
            continue;
        }
        result.supporting_evidence.push_back(item);
        const std::string dependency = tonal_region_dependency_key(item);
        auto found = best_support_by_dependency.find(dependency);
        if (found == best_support_by_dependency.end() || item.confidence > found->second) {
            best_support_by_dependency[dependency] = item.confidence;
            origin_by_dependency[dependency] = item.origin;
        }
    }

    std::vector<double> independent_strengths;
    std::set<tonal_region_relation_evidence_origin> independent_origins;
    for (const auto& item : best_support_by_dependency) {
        independent_strengths.push_back(item.second);
        independent_origins.insert(origin_by_dependency[item.first]);
    }
    std::sort(independent_strengths.begin(), independent_strengths.end(), std::greater<double>{});
    result.independent_support_groups = independent_strengths.size();
    result.independent_support_origins = independent_origins.size();
    result.independent_support_ceiling = independent_strengths.empty()
        ? 0.0
        : independent_strengths.size() == 1
            ? independent_strengths[0]
            : independent_strengths[1];

    const double base_confidence = std::min(source_center.confidence, target_center.confidence);
    const bool target_grounded = target_center.cross_origin_grounded &&
        target_center.confidence >= tonal_region_min_grounded_center_confidence;
    const bool phrase_partition = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::phrase_partition,
        0.60);
    const bool structural_arrival = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::structural_arrival,
        0.60);
    const bool persistence = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::center_persistence,
        0.60);
    const bool structural_collection = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::structural_pitch_collection,
        0.60) || result.target_key_class_resolved;
    const bool return_reentry = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::return_reentry,
        0.60);
    const bool form_recurrence = has_relation_evidence_kind(
        result.supporting_evidence,
        tonal_region_relation_evidence_kind::form_recurrence,
        0.60);

    if (result.centers_equivalent) {
        result.kind = tonal_region_relation_kind::retained_center;
        result.confidence = std::min(base_confidence, tonal_region_retained_center_ceiling);
    } else {
        result.kind = tonal_region_relation_kind::contrasting_center;
        result.confidence = std::min(base_confidence, tonal_region_contrast_ceiling);

        if (result.topology == tonal_region_topology::target_nested_in_source &&
            target_grounded &&
            result.independent_support_groups >= 2 &&
            result.independent_support_origins >= 2 &&
            structural_arrival &&
            (structural_collection || persistence)) {
            result.kind = tonal_region_relation_kind::tonicization_candidate;
            result.confidence = std::min(
                {base_confidence,
                 result.independent_support_ceiling,
                 tonal_region_tonicization_candidate_ceiling});
        }

        if (result.topology == tonal_region_topology::sequential &&
            target_grounded &&
            result.independent_support_groups >= 3 &&
            result.independent_support_origins >= 2 &&
            phrase_partition &&
            structural_arrival &&
            persistence &&
            structural_collection) {
            result.kind = tonal_region_relation_kind::modulation_candidate;
            const double third_support = independent_strengths.size() >= 3
                ? independent_strengths[2]
                : 0.0;
            result.confidence = std::min(
                {base_confidence,
                 result.independent_support_ceiling,
                 third_support,
                 tonal_region_modulation_candidate_ceiling});
        }

        if (result.topology == tonal_region_topology::sequential &&
            result.return_reference_supplied &&
            result.target_matches_return_reference &&
            target_grounded &&
            return_reentry &&
            form_recurrence &&
            structural_arrival &&
            result.independent_support_groups >= 3 &&
            result.independent_support_origins >= 2) {
            result.kind = tonal_region_relation_kind::return_candidate;
            const double third_support = independent_strengths.size() >= 3
                ? independent_strengths[2]
                : 0.0;
            result.confidence = std::min(
                {base_confidence,
                 result.independent_support_ceiling,
                 third_support,
                 tonal_region_return_candidate_ceiling});
        }
    }

    if (result.strong_counterevidence)
        result.confidence = std::min(result.confidence, tonal_region_strong_conflict_ceiling);

    // Candidate relation labels remain hypotheses. This layer does not establish
    // functional modulation, tonicization, pivot function or Roman numerals.
    result.modulation_established = false;
    result.tonicization_established = false;
    return result;
}

inline node_id add_tonal_region_relation_hypothesis(
    musical_execution_graph& graph,
    const tonal_region_relation_hypothesis& hypothesis) {
    for (const auto& item : hypothesis.supporting_evidence) {
        if (item.source_node != 0 && graph.find_node(item.source_node) == nullptr)
            throw std::invalid_argument("tonal-region relation references an unknown support node");
    }

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "tonal region relation hypothesis";
    const time_coordinate relation_end =
        hypothesis.source_region.end->tick >= hypothesis.target_region.end->tick
            ? *hypothesis.source_region.end
            : *hypothesis.target_region.end;
    relation.active = time_span{hypothesis.source_region.start, relation_end};
    relation.attributes.push_back({
        "identity_scope",
        std::string{"tonal_region_relation_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "relation_kind",
        std::string{to_string(hypothesis.kind)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "topology",
        std::string{to_string(hypothesis.topology)},
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "center_distance_octaves",
        hypothesis.center_distance_octaves,
        evidence_status::derived,
        1.0,
        "octaves modulo 1",
    });
    relation.attributes.push_back({
        "target_key_class_resolved",
        hypothesis.target_key_class_resolved,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "modulation_established",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "tonicization_established",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        "hierarchical tonal-region evidence",
        std::nullopt,
        "relation among grounded tonal-center regions; candidate labels do not by themselves establish functional modulation, tonicization, pivot function, or Roman-numeral analysis",
    });
    const node_id relation_id = graph.add_node(std::move(relation));

    for (const auto& item : hypothesis.supporting_evidence) {
        if (item.source_node == 0)
            continue;
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = item.source_node;
        support.to = relation_id;
        support.attributes.push_back({
            "support_role",
            std::string{to_string(item.kind)},
            item.status,
            item.confidence,
            item.source,
        });
        graph.add_edge(std::move(support));
    }
    return relation_id;
}

} // namespace vgmtooling::model
