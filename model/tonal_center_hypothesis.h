#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class tonal_center_evidence_kind : std::uint8_t {
    recurrence = 0,
    duration,
    bass_support,
    structural_arrival,
    harmonic_stability,
    authored_annotation,
    contradiction,
};

enum class tonal_center_evidence_origin : std::uint8_t {
    pitch_distribution = 0,
    bass_structure,
    phrase_structure,
    harmony,
    authored_source,
    external_annotation,
};

struct tonal_center_evidence {
    tonal_center_evidence_kind kind = tonal_center_evidence_kind::recurrence;
    tonal_center_evidence_origin origin = tonal_center_evidence_origin::pitch_distribution;
    double center_octave_class = 0.0;
    double confidence = 0.0;
    std::string dependency_group;
    node_id source_node = 0;
    bool supports_candidate = true;
    evidence_status status = evidence_status::derived;
    std::string source;
};

struct tonal_center_hypothesis {
    double center_octave_class = 0.0;
    double tolerance_octaves = 0.0;
    time_span region{};
    std::size_t independent_support_groups = 0;
    std::size_t independent_support_origins = 0;
    double independent_support_ceiling = 0.0;
    bool cross_origin_grounded = false;
    bool strong_counterevidence = false;
    bool key_named = false;
    bool mode_named = false;
    bool tonal_function_named = false;
    double confidence = 0.0;
    std::vector<tonal_center_evidence> supporting_evidence;
    std::vector<tonal_center_evidence> counterevidence;
};

constexpr double tonal_center_single_support_ceiling = 0.45;
constexpr double tonal_center_single_origin_ceiling = 0.60;
constexpr double tonal_center_two_group_ceiling = 0.69;
constexpr double tonal_center_three_group_ceiling = 0.82;
constexpr double tonal_center_four_group_ceiling = 0.90;
constexpr double tonal_center_strong_conflict_ceiling = 0.49;
constexpr double tonal_center_strong_counterevidence_threshold = 0.70;
constexpr double default_tonal_center_tolerance_octaves = 35.0 / 1200.0;

inline const char* to_string(tonal_center_evidence_kind kind) noexcept {
    switch (kind) {
    case tonal_center_evidence_kind::recurrence:
        return "recurrence";
    case tonal_center_evidence_kind::duration:
        return "duration";
    case tonal_center_evidence_kind::bass_support:
        return "bass_support";
    case tonal_center_evidence_kind::structural_arrival:
        return "structural_arrival";
    case tonal_center_evidence_kind::harmonic_stability:
        return "harmonic_stability";
    case tonal_center_evidence_kind::authored_annotation:
        return "authored_annotation";
    case tonal_center_evidence_kind::contradiction:
        return "contradiction";
    }
    return "unknown";
}

inline const char* to_string(tonal_center_evidence_origin origin) noexcept {
    switch (origin) {
    case tonal_center_evidence_origin::pitch_distribution:
        return "pitch_distribution";
    case tonal_center_evidence_origin::bass_structure:
        return "bass_structure";
    case tonal_center_evidence_origin::phrase_structure:
        return "phrase_structure";
    case tonal_center_evidence_origin::harmony:
        return "harmony";
    case tonal_center_evidence_origin::authored_source:
        return "authored_source";
    case tonal_center_evidence_origin::external_annotation:
        return "external_annotation";
    }
    return "unknown";
}

inline double normalize_octave_class(double value) {
    if (!std::isfinite(value))
        throw std::invalid_argument("tonal-center octave class must be finite");
    double normalized = std::fmod(value, 1.0);
    if (normalized < 0.0)
        normalized += 1.0;
    if (normalized >= 1.0)
        normalized = 0.0;
    return normalized;
}

inline double circular_octave_class_distance(double first, double second) {
    const double a = normalize_octave_class(first);
    const double b = normalize_octave_class(second);
    const double direct = std::fabs(a - b);
    return std::min(direct, 1.0 - direct);
}

inline std::string tonal_center_dependency_key(const tonal_center_evidence& evidence) {
    if (!evidence.dependency_group.empty())
        return evidence.dependency_group;
    // Empty dependency groups are deliberately conservative: evidence from the
    // same origin collapses into one vote rather than becoming independent by
    // accident.
    return std::string{"origin:"} + to_string(evidence.origin);
}

inline tonal_center_hypothesis infer_tonal_center_hypothesis(
    double candidate_octave_class,
    const time_span& region,
    std::vector<tonal_center_evidence> evidence,
    double tolerance_octaves = default_tonal_center_tolerance_octaves) {
    if (!std::isfinite(tolerance_octaves) || tolerance_octaves <= 0.0 || tolerance_octaves >= 0.5)
        throw std::invalid_argument("tonal-center tolerance must be finite and between zero and half an octave");

    tonal_center_hypothesis result;
    result.center_octave_class = normalize_octave_class(candidate_octave_class);
    result.tolerance_octaves = tolerance_octaves;
    result.region = region;

    std::map<std::string, double> best_support_by_dependency;
    std::map<std::string, tonal_center_evidence_origin> support_origin_by_dependency;

    for (auto& item : evidence) {
        if (!std::isfinite(item.confidence) || item.confidence < 0.0 || item.confidence > 1.0)
            throw std::invalid_argument("tonal-center evidence confidence must lie in [0, 1]");
        item.center_octave_class = normalize_octave_class(item.center_octave_class);

        const double distance = circular_octave_class_distance(
            result.center_octave_class,
            item.center_octave_class);
        if (distance > tolerance_octaves)
            continue;

        if (!item.supports_candidate || item.kind == tonal_center_evidence_kind::contradiction) {
            result.counterevidence.push_back(item);
            if (item.confidence >= tonal_center_strong_counterevidence_threshold)
                result.strong_counterevidence = true;
            continue;
        }

        const double geometric_fit = 1.0 - (distance / tolerance_octaves);
        const double effective_confidence = item.confidence * geometric_fit;
        result.supporting_evidence.push_back(item);

        const std::string dependency = tonal_center_dependency_key(item);
        auto found = best_support_by_dependency.find(dependency);
        if (found == best_support_by_dependency.end() || effective_confidence > found->second) {
            best_support_by_dependency[dependency] = effective_confidence;
            support_origin_by_dependency[dependency] = item.origin;
        }
    }

    if (best_support_by_dependency.empty())
        return result;

    std::vector<double> independent_strengths;
    independent_strengths.reserve(best_support_by_dependency.size());
    std::set<tonal_center_evidence_origin> independent_origins;
    for (const auto& item : best_support_by_dependency) {
        independent_strengths.push_back(item.second);
        independent_origins.insert(support_origin_by_dependency[item.first]);
    }
    std::sort(independent_strengths.begin(), independent_strengths.end(), std::greater<double>{});

    result.independent_support_groups = independent_strengths.size();
    result.independent_support_origins = independent_origins.size();
    result.cross_origin_grounded = independent_origins.size() >= 2;

    if (independent_strengths.size() == 1) {
        result.independent_support_ceiling = tonal_center_single_support_ceiling;
        result.confidence = std::min(
            independent_strengths[0],
            tonal_center_single_support_ceiling);
    } else if (independent_strengths.size() == 2) {
        result.independent_support_ceiling = std::min(
            {independent_strengths[0], independent_strengths[1], tonal_center_two_group_ceiling});
        result.confidence = result.independent_support_ceiling;
    } else if (independent_strengths.size() == 3) {
        result.independent_support_ceiling = std::min(
            {independent_strengths[0], independent_strengths[1], independent_strengths[2], tonal_center_three_group_ceiling});
        result.confidence = result.independent_support_ceiling;
    } else {
        result.independent_support_ceiling = std::min(
            {independent_strengths[0], independent_strengths[1], independent_strengths[2], independent_strengths[3], tonal_center_four_group_ceiling});
        result.confidence = result.independent_support_ceiling;
    }

    if (!result.cross_origin_grounded)
        result.confidence = std::min(result.confidence, tonal_center_single_origin_ceiling);
    if (result.strong_counterevidence)
        result.confidence = std::min(result.confidence, tonal_center_strong_conflict_ceiling);

    // A tonal center is intentionally weaker than a key or a named function.
    // Those labels require later mode/scale and functional evidence.
    result.key_named = false;
    result.mode_named = false;
    result.tonal_function_named = false;
    return result;
}

inline node_id add_tonal_center_hypothesis(
    musical_execution_graph& graph,
    const tonal_center_hypothesis& hypothesis) {
    for (const auto& item : hypothesis.supporting_evidence) {
        if (item.source_node != 0 && graph.find_node(item.source_node) == nullptr)
            throw std::invalid_argument("tonal-center hypothesis references an unknown support node");
    }
    for (const auto& item : hypothesis.counterevidence) {
        if (item.source_node != 0 && graph.find_node(item.source_node) == nullptr)
            throw std::invalid_argument("tonal-center hypothesis references an unknown counterevidence node");
    }

    node center;
    center.kind = node_kind::pattern;
    center.layer = semantic_layer::musical_structure;
    center.flow = flow_kind::value;
    center.label = "tonal center hypothesis";
    center.active = hypothesis.region;
    center.attributes.push_back({
        "identity_scope",
        std::string{"tonal_center_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    center.attributes.push_back({
        "center_octave_class",
        hypothesis.center_octave_class,
        evidence_status::hypothesis,
        hypothesis.confidence,
        "octaves modulo 1",
    });
    center.attributes.push_back({
        "independent_support_groups",
        static_cast<std::uint64_t>(hypothesis.independent_support_groups),
        evidence_status::derived,
        1.0,
        "groups",
    });
    center.attributes.push_back({
        "independent_support_origins",
        static_cast<std::uint64_t>(hypothesis.independent_support_origins),
        evidence_status::derived,
        1.0,
        "origins",
    });
    center.attributes.push_back({
        "cross_origin_grounded",
        hypothesis.cross_origin_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    center.attributes.push_back({
        "key_named",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    center.attributes.push_back({
        "mode_named",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    center.attributes.push_back({
        "tonal_function_named",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    center.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        "tonal-center evidence aggregation",
        std::nullopt,
        "octave-equivalent structural center only; this does not establish key, mode, tonic/dominant function, cadence class, or enharmonic spelling",
    });
    const node_id center_id = graph.add_node(std::move(center));

    for (const auto& item : hypothesis.supporting_evidence) {
        if (item.source_node == 0)
            continue;
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = item.source_node;
        support.to = center_id;
        support.attributes.push_back({
            "support_role",
            std::string{to_string(item.kind)},
            item.status,
            item.confidence,
            item.source,
        });
        graph.add_edge(std::move(support));
    }
    return center_id;
}

} // namespace vgmtooling::model
