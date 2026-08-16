#pragma once

#include "tonal_center_evidence_adapter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class diatonic_mode : std::uint8_t {
    ionian = 0,
    dorian,
    phrygian,
    lydian,
    mixolydian,
    aeolian,
    locrian,
};

enum class pitch_class_collection_scope : std::uint8_t {
    surface_performance = 0,
    structural_hypothesis,
    authored_annotation,
};

struct pitch_class_collection_profile {
    time_span region{};
    equal_temperament_model tuning{};
    musical_pitch_role pitch_role = musical_pitch_role::programmed;
    pitch_class_collection_scope scope = pitch_class_collection_scope::surface_performance;
    std::array<double, 12> salience{};
    double projection_coverage = 1.0;
    double confidence = 0.0;
    std::string source;
};

struct diatonic_mode_candidate {
    diatonic_mode mode = diatonic_mode::ionian;
    std::int64_t center_pitch_class = 0;
    double in_collection_fit = 0.0;
    double template_coverage = 0.0;
    double composite_score = 0.0;
    double confidence = 0.0;
};

struct tonal_key_class_hypothesis {
    time_span region{};
    double center_octave_class = 0.0;
    std::int64_t center_pitch_class = 0;
    std::optional<diatonic_mode> mode{};
    std::vector<diatonic_mode_candidate> alternatives;
    double best_score = 0.0;
    double runner_up_score = 0.0;
    double separation = 0.0;
    std::size_t distinct_pitch_classes = 0;
    pitch_class_collection_scope collection_scope = pitch_class_collection_scope::surface_performance;
    bool center_cross_origin_grounded = false;
    bool key_class_resolved = false;
    bool enharmonic_spelling_named = false;
    bool tonal_function_named = false;
    double confidence = 0.0;
    std::string theory_scope = "12-TET diatonic seven-mode";
};

constexpr std::size_t diatonic_key_min_distinct_pitch_classes = 6;
constexpr double diatonic_key_min_center_confidence = 0.69;
constexpr double diatonic_key_min_collection_confidence = 0.70;
constexpr double diatonic_key_min_projection_coverage = 0.85;
constexpr double diatonic_key_min_fit = 0.85;
constexpr double diatonic_key_min_coverage = 6.0 / 7.0;
constexpr double diatonic_key_min_separation = 0.12;
constexpr double diatonic_key_confidence_ceiling = 0.90;
constexpr double diatonic_center_projection_tolerance_octaves = 35.0 / 1200.0;

inline const char* to_string(diatonic_mode mode) noexcept {
    switch (mode) {
    case diatonic_mode::ionian: return "ionian";
    case diatonic_mode::dorian: return "dorian";
    case diatonic_mode::phrygian: return "phrygian";
    case diatonic_mode::lydian: return "lydian";
    case diatonic_mode::mixolydian: return "mixolydian";
    case diatonic_mode::aeolian: return "aeolian";
    case diatonic_mode::locrian: return "locrian";
    }
    return "unknown";
}

inline const char* to_string(pitch_class_collection_scope scope) noexcept {
    switch (scope) {
    case pitch_class_collection_scope::surface_performance: return "surface_performance";
    case pitch_class_collection_scope::structural_hypothesis: return "structural_hypothesis";
    case pitch_class_collection_scope::authored_annotation: return "authored_annotation";
    }
    return "unknown";
}

inline std::array<std::int64_t, 7> diatonic_mode_template(diatonic_mode mode) {
    switch (mode) {
    case diatonic_mode::ionian: return {0, 2, 4, 5, 7, 9, 11};
    case diatonic_mode::dorian: return {0, 2, 3, 5, 7, 9, 10};
    case diatonic_mode::phrygian: return {0, 1, 3, 5, 7, 8, 10};
    case diatonic_mode::lydian: return {0, 2, 4, 6, 7, 9, 11};
    case diatonic_mode::mixolydian: return {0, 2, 4, 5, 7, 9, 10};
    case diatonic_mode::aeolian: return {0, 2, 3, 5, 7, 8, 10};
    case diatonic_mode::locrian: return {0, 1, 3, 5, 6, 8, 10};
    }
    return {0, 2, 4, 5, 7, 9, 11};
}

inline void validate_pitch_class_collection_profile(const pitch_class_collection_profile& profile) {
    if (profile.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("diatonic key inference currently requires explicit 12-TET pitch classes");
    validate_equal_temperament_model(profile.tuning);
    if (!std::isfinite(profile.confidence) || profile.confidence < 0.0 || profile.confidence > 1.0)
        throw std::invalid_argument("pitch-class collection confidence must lie in [0, 1]");
    if (!std::isfinite(profile.projection_coverage) || profile.projection_coverage < 0.0 || profile.projection_coverage > 1.0)
        throw std::invalid_argument("pitch-class projection coverage must lie in [0, 1]");
    if (profile.source.empty())
        throw std::invalid_argument("pitch-class collection requires provenance source");
    double total = 0.0;
    for (double value : profile.salience) {
        if (!std::isfinite(value) || value < 0.0)
            throw std::invalid_argument("pitch-class salience must be finite and nonnegative");
        total += value;
    }
    if (total <= 0.0)
        throw std::invalid_argument("pitch-class collection requires positive salience");
}

inline bool time_span_contains_span(const time_span& outer, const time_span& inner) noexcept {
    if (outer.start.domain != inner.start.domain ||
        outer.start.tick_rate != inner.start.tick_rate ||
        outer.start.loop_iteration != inner.start.loop_iteration ||
        inner.start.tick < outer.start.tick) return false;
    if (inner.end.has_value()) {
        if (inner.end->domain != inner.start.domain ||
            inner.end->tick_rate != inner.start.tick_rate ||
            inner.end->loop_iteration != inner.start.loop_iteration) return false;
    }
    if (!outer.end.has_value()) return true;
    if (!inner.end.has_value()) return false;
    return inner.end->tick <= outer.end->tick;
}

inline std::int64_t nearest_12tet_pitch_class_for_center(
    double center_octave_class,
    const equal_temperament_model& tuning,
    double tolerance_octaves = diatonic_center_projection_tolerance_octaves) {
    if (tuning.divisions_per_octave != 12)
        throw std::invalid_argument("diatonic center projection requires 12-TET");
    if (!std::isfinite(tolerance_octaves) || tolerance_octaves <= 0.0 || tolerance_octaves >= 0.5)
        throw std::invalid_argument("diatonic center projection tolerance is invalid");
    std::int64_t best_pitch_class = 0;
    double best_distance = 1.0;
    for (std::int64_t pitch_class = 0; pitch_class < 12; ++pitch_class) {
        const double candidate = equal_temperament_step_octave_class(tuning, pitch_class);
        const double distance = circular_octave_class_distance(center_octave_class, candidate);
        if (distance < best_distance) {
            best_distance = distance;
            best_pitch_class = pitch_class;
        }
    }
    if (best_distance > tolerance_octaves)
        throw std::invalid_argument("tonal center does not fit the supplied 12-TET tuning contract");
    return best_pitch_class;
}

inline std::vector<diatonic_mode_candidate> infer_diatonic_mode_candidates(
    const tonal_center_hypothesis& center,
    const pitch_class_collection_profile& collection) {
    validate_pitch_class_collection_profile(collection);
    if (!time_span_contains_span(center.region, collection.region))
        throw std::invalid_argument("pitch-class collection must lie inside the supplied tonal-center region");
    const std::int64_t center_pitch_class = nearest_12tet_pitch_class_for_center(
        center.center_octave_class, collection.tuning);
    double total_salience = 0.0;
    for (double value : collection.salience) total_salience += value;

    std::vector<diatonic_mode_candidate> candidates;
    candidates.reserve(7);
    for (diatonic_mode mode : {diatonic_mode::ionian, diatonic_mode::dorian,
             diatonic_mode::phrygian, diatonic_mode::lydian,
             diatonic_mode::mixolydian, diatonic_mode::aeolian,
             diatonic_mode::locrian}) {
        std::array<bool, 12> member{};
        const auto relative_template = diatonic_mode_template(mode);
        for (std::int64_t relative : relative_template)
            member[static_cast<std::size_t>(positive_mod(center_pitch_class + relative, 12))] = true;
        double in_collection_salience = 0.0;
        std::size_t covered_template_classes = 0;
        for (std::size_t pitch_class = 0; pitch_class < collection.salience.size(); ++pitch_class) {
            if (member[pitch_class]) {
                in_collection_salience += collection.salience[pitch_class];
                if (collection.salience[pitch_class] > 0.0) ++covered_template_classes;
            }
        }
        diatonic_mode_candidate candidate;
        candidate.mode = mode;
        candidate.center_pitch_class = center_pitch_class;
        candidate.in_collection_fit = in_collection_salience / total_salience;
        candidate.template_coverage = static_cast<double>(covered_template_classes) / 7.0;
        candidate.composite_score = candidate.in_collection_fit * candidate.template_coverage;
        candidate.confidence = std::min({center.confidence, collection.confidence,
            collection.projection_coverage, candidate.in_collection_fit,
            candidate.template_coverage});
        candidates.push_back(candidate);
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& first, const auto& second) {
        if (first.composite_score != second.composite_score)
            return first.composite_score > second.composite_score;
        return static_cast<std::uint8_t>(first.mode) < static_cast<std::uint8_t>(second.mode);
    });
    return candidates;
}

inline tonal_key_class_hypothesis infer_tonal_key_class_hypothesis(
    const tonal_center_hypothesis& center,
    const pitch_class_collection_profile& collection) {
    auto candidates = infer_diatonic_mode_candidates(center, collection);
    if (candidates.empty())
        throw std::logic_error("diatonic mode candidate set unexpectedly empty");
    tonal_key_class_hypothesis result;
    result.region = collection.region;
    result.center_octave_class = center.center_octave_class;
    result.center_pitch_class = candidates.front().center_pitch_class;
    result.collection_scope = collection.scope;
    result.center_cross_origin_grounded = center.cross_origin_grounded;
    result.alternatives = std::move(candidates);
    result.best_score = result.alternatives.front().composite_score;
    result.runner_up_score = result.alternatives.size() >= 2 ? result.alternatives[1].composite_score : 0.0;
    result.separation = result.best_score - result.runner_up_score;
    for (double value : collection.salience)
        result.distinct_pitch_classes += value > 0.0 ? 1u : 0u;

    const auto& best = result.alternatives.front();
    const bool structural_collection =
        collection.scope == pitch_class_collection_scope::structural_hypothesis ||
        collection.scope == pitch_class_collection_scope::authored_annotation;
    const bool enough_center = center.cross_origin_grounded &&
        center.confidence >= diatonic_key_min_center_confidence;
    const bool enough_collection = structural_collection &&
        collection.confidence >= diatonic_key_min_collection_confidence &&
        collection.projection_coverage >= diatonic_key_min_projection_coverage &&
        result.distinct_pitch_classes >= diatonic_key_min_distinct_pitch_classes;
    const bool enough_fit = best.in_collection_fit >= diatonic_key_min_fit &&
        best.template_coverage >= diatonic_key_min_coverage;
    const bool distinct_winner = result.separation >= diatonic_key_min_separation;

    if (enough_center && enough_collection && enough_fit && distinct_winner) {
        const double separation_confidence = std::min(1.0,
            result.separation / diatonic_key_min_separation);
        result.mode = best.mode;
        result.key_class_resolved = true;
        result.confidence = std::min({best.confidence, center.confidence,
            collection.confidence, collection.projection_coverage,
            separation_confidence, diatonic_key_confidence_ceiling});
    }
    result.enharmonic_spelling_named = false;
    result.tonal_function_named = false;
    return result;
}

inline node_id add_tonal_key_class_hypothesis(
    musical_execution_graph& graph,
    const tonal_key_class_hypothesis& hypothesis) {
    if (!hypothesis.key_class_resolved || !hypothesis.mode.has_value())
        throw std::invalid_argument("only a resolved tonal key class may be materialized as a key hypothesis");
    node key;
    key.kind = node_kind::pattern;
    key.layer = semantic_layer::musical_structure;
    key.flow = flow_kind::value;
    key.label = "tonal key-class hypothesis";
    key.active = hypothesis.region;
    key.attributes.push_back({"identity_scope", std::string{"tonal_key_class_hypothesis"},
        evidence_status::hypothesis, hypothesis.confidence, ""});
    key.attributes.push_back({"center_pitch_class", hypothesis.center_pitch_class,
        evidence_status::hypothesis, hypothesis.confidence, "12-TET pitch class"});
    key.attributes.push_back({"mode", std::string{to_string(*hypothesis.mode)},
        evidence_status::hypothesis, hypothesis.confidence, ""});
    key.attributes.push_back({"collection_scope", std::string{to_string(hypothesis.collection_scope)},
        evidence_status::derived, 1.0, ""});
    key.attributes.push_back({"theory_scope", hypothesis.theory_scope,
        evidence_status::derived, 1.0, ""});
    key.attributes.push_back({"enharmonic_spelling_named", false,
        evidence_status::derived, 1.0, ""});
    key.attributes.push_back({"tonal_function_named", false,
        evidence_status::derived, 1.0, ""});
    key.provenance.push_back({evidence_status::hypothesis, hypothesis.confidence,
        "tonal center + structurally grounded pitch-class collection", std::nullopt,
        "theory-scoped 12-TET diatonic key class; preserves competing mode candidates and does not establish enharmonic spelling or Roman-numeral function"});
    return graph.add_node(std::move(key));
}

} // namespace vgmtooling::model
