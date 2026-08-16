#pragma once

#include "diatonic_key_hypothesis.h"
#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

struct diatonic_chord_degree_hypothesis {
    time_coordinate observation_time{};
    std::int64_t key_center_pitch_class = 0;
    diatonic_mode key_mode = diatonic_mode::ionian;
    std::int64_t chord_root_pitch_class = 0;
    std::int64_t relative_root_semitones = 0;
    std::optional<std::uint8_t> scale_degree{};
    tertian_triad_quality observed_quality = tertian_triad_quality::major;
    std::optional<tertian_triad_quality> expected_diatonic_quality{};
    bool root_in_diatonic_collection = false;
    bool quality_matches_diatonic_stack = false;
    bool chromatic_root = false;
    bool roman_numeral_named = false;
    bool tonal_function_named = false;
    double confidence = 0.0;
    std::string theory_scope = "12-TET diatonic seven-mode chord degree";
};

inline std::optional<tertian_triad_quality> diatonic_stacked_triad_quality(
    diatonic_mode mode,
    std::size_t degree_index) {
    if (degree_index >= 7)
        throw std::invalid_argument("diatonic degree index must lie in [0, 6]");
    const auto scale = diatonic_mode_template(mode);
    const std::int64_t root = scale[degree_index];
    const std::int64_t third = positive_mod(scale[(degree_index + 2) % 7] - root, 12);
    const std::int64_t fifth = positive_mod(scale[(degree_index + 4) % 7] - root, 12);
    const std::set<std::int64_t> observed = {0, third, fifth};

    for (tertian_triad_quality quality : {
             tertian_triad_quality::major,
             tertian_triad_quality::minor,
             tertian_triad_quality::diminished,
             tertian_triad_quality::augmented,
         }) {
        std::set<std::int64_t> candidate;
        for (std::int64_t offset : triad_template(quality))
            candidate.insert(offset);
        if (candidate == observed)
            return quality;
    }
    return std::nullopt;
}

inline diatonic_chord_degree_hypothesis infer_diatonic_chord_degree_hypothesis(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& chord) {
    if (!key.key_class_resolved || !key.mode.has_value())
        throw std::invalid_argument("chord degree requires a resolved tonal key class");
    validate_equal_temperament_model(key.tuning);
    validate_equal_temperament_model(chord.projection.tuning);
    if (key.tuning.divisions_per_octave != 12 ||
        chord.projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("diatonic chord degree currently requires explicit 12-TET contracts");
    if (!compatible_tonal_center_tuning(key.tuning, chord.projection.tuning))
        throw std::invalid_argument("chord degree cannot compare incompatible tuning contracts");
    if (key.pitch_role != chord.projection.source_verticality.role)
        throw std::invalid_argument("chord degree cannot mix programmed, performed, and heard pitch roles");
    if (!time_coordinate_inside_span(
            chord.projection.source_verticality.observation_time,
            key.region))
        throw std::invalid_argument("chord observation lies outside resolved key-class region");
    if (chord.root_ambiguous)
        throw std::invalid_argument("ambiguous chord root cannot establish a scale-degree relation");
    if (chord.confidence < 0.0 || chord.confidence > 1.0)
        throw std::invalid_argument("chord confidence must lie in [0, 1]");

    diatonic_chord_degree_hypothesis result;
    result.observation_time = chord.projection.source_verticality.observation_time;
    result.key_center_pitch_class = positive_mod(key.center_pitch_class, 12);
    result.key_mode = *key.mode;
    result.chord_root_pitch_class = positive_mod(chord.root_pitch_class, 12);
    result.relative_root_semitones = positive_mod(
        result.chord_root_pitch_class - result.key_center_pitch_class,
        12);
    result.observed_quality = chord.quality;
    result.confidence = std::min(
        {key.confidence, chord.confidence, chord.projection.confidence,
         key.tuning.confidence, chord.projection.tuning.confidence});

    const auto scale = diatonic_mode_template(result.key_mode);
    for (std::size_t index = 0; index < scale.size(); ++index) {
        if (scale[index] != result.relative_root_semitones)
            continue;
        result.root_in_diatonic_collection = true;
        result.scale_degree = static_cast<std::uint8_t>(index + 1);
        result.expected_diatonic_quality = diatonic_stacked_triad_quality(
            result.key_mode,
            index);
        result.quality_matches_diatonic_stack =
            result.expected_diatonic_quality.has_value() &&
            *result.expected_diatonic_quality == chord.quality;
        break;
    }
    result.chromatic_root = !result.root_in_diatonic_collection;

    // Degree and quality are collection-relative structural facts. Neither a
    // Roman numeral nor a functional label is earned at this layer.
    result.roman_numeral_named = false;
    result.tonal_function_named = false;
    return result;
}

inline node_id add_diatonic_chord_degree_hypothesis(
    musical_execution_graph& graph,
    const diatonic_chord_degree_hypothesis& hypothesis,
    node_id key_node,
    node_id chord_node) {
    if (graph.find_node(key_node) == nullptr || graph.find_node(chord_node) == nullptr)
        throw std::invalid_argument("chord-degree relation requires materialized key and chord nodes");

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "diatonic chord-degree hypothesis";
    relation.active = time_span{hypothesis.observation_time, hypothesis.observation_time};
    relation.attributes.push_back({"identity_scope",
        std::string{"diatonic_chord_degree_hypothesis"}, evidence_status::hypothesis,
        hypothesis.confidence, ""});
    relation.attributes.push_back({"key_center_pitch_class", hypothesis.key_center_pitch_class,
        evidence_status::hypothesis, hypothesis.confidence, "12-TET pitch class"});
    relation.attributes.push_back({"key_mode", std::string{to_string(hypothesis.key_mode)},
        evidence_status::hypothesis, hypothesis.confidence, ""});
    relation.attributes.push_back({"chord_root_pitch_class", hypothesis.chord_root_pitch_class,
        evidence_status::hypothesis, hypothesis.confidence, "12-TET pitch class"});
    relation.attributes.push_back({"chromatic_root", hypothesis.chromatic_root,
        evidence_status::derived, 1.0, ""});
    if (hypothesis.scale_degree.has_value()) {
        relation.attributes.push_back({"scale_degree",
            static_cast<std::uint64_t>(*hypothesis.scale_degree),
            evidence_status::hypothesis, hypothesis.confidence, "1-based diatonic degree"});
    }
    relation.attributes.push_back({"quality_matches_diatonic_stack",
        hypothesis.quality_matches_diatonic_stack,
        evidence_status::hypothesis, hypothesis.confidence, ""});
    relation.attributes.push_back({"roman_numeral_named", false,
        evidence_status::derived, 1.0, ""});
    relation.attributes.push_back({"tonal_function_named", false,
        evidence_status::derived, 1.0, ""});
    relation.provenance.push_back({evidence_status::hypothesis, hypothesis.confidence,
        "resolved key-class + unambiguous chord root under retained tuning/pitch-role contracts",
        std::nullopt,
        "scale degree and diatonic chord quality only; this does not establish Roman numeral, dominant/tonic function, secondary function, pivot status, or cadence class"});
    const node_id relation_id = graph.add_node(std::move(relation));

    edge key_support;
    key_support.kind = edge_kind::derived_from;
    key_support.from = key_node;
    key_support.to = relation_id;
    graph.add_edge(std::move(key_support));

    edge chord_support;
    chord_support.kind = edge_kind::derived_from;
    chord_support.from = chord_node;
    chord_support.to = relation_id;
    graph.add_edge(std::move(chord_support));
    return relation_id;
}

} // namespace vgmtooling::model
