#pragma once

#include "bass_harmony_interaction.h"
#include "cadential_arrival_hypothesis.h"
#include "diatonic_chord_degree_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

enum class ionian_functional_tendency_kind : std::uint8_t {
    unresolved = 0,
    predominant_progression_candidate,
    dominant_resolution_candidate,
};

struct ionian_functional_tendency_hypothesis {
    ionian_functional_tendency_kind kind = ionian_functional_tendency_kind::unresolved;
    time_coordinate first_time{};
    time_coordinate second_time{};
    std::optional<std::uint8_t> source_degree{};
    std::optional<std::uint8_t> target_degree{};
    tertian_triad_quality source_quality = tertian_triad_quality::major;
    tertian_triad_quality target_quality = tertian_triad_quality::major;
    bool source_quality_diatonic = false;
    bool target_quality_diatonic = false;
    bool root_motion_reliable = false;
    bool voice_leading_supplied = false;
    bool voice_leading_identity_grounded = false;
    bool bass_evidence_supplied = false;
    bool bass_identity_grounded = false;
    bool phrase_arrival_supplied = false;
    bool phrase_arrival_cross_part_grounded = false;
    bool candidate_resolved = false;
    bool tonal_function_established = false;
    bool roman_numeral_named = false;
    double confidence = 0.0;
    std::string theory_scope = "12-TET Ionian common-practice functional tendency";
};

constexpr double ionian_function_degree_transition_ceiling = 0.60;
constexpr double ionian_function_inferred_voice_ceiling = 0.69;
constexpr double ionian_function_identity_voice_ceiling = 0.74;
constexpr double ionian_function_phrase_arrival_ceiling = 0.82;

inline const char* to_string(ionian_functional_tendency_kind kind) noexcept {
    switch (kind) {
    case ionian_functional_tendency_kind::unresolved:
        return "unresolved";
    case ionian_functional_tendency_kind::predominant_progression_candidate:
        return "predominant_progression_candidate";
    case ionian_functional_tendency_kind::dominant_resolution_candidate:
        return "dominant_resolution_candidate";
    }
    return "unknown";
}

inline bool same_function_transition_time(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration &&
           first.tick == second.tick;
}

inline bool ionian_predominant_degree(std::uint8_t degree) noexcept {
    return degree == 2 || degree == 4;
}

inline bool ionian_dominant_degree(std::uint8_t degree) noexcept {
    return degree == 5 || degree == 7;
}

inline ionian_functional_tendency_hypothesis infer_ionian_functional_tendency(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& first_chord,
    const tertian_triad_hypothesis& second_chord,
    const std::optional<voice_leading_hypothesis>& voices = std::nullopt,
    const std::optional<bass_harmony_interaction_hypothesis>& bass = std::nullopt,
    const std::optional<cadential_arrival_hypothesis>& arrival = std::nullopt) {
    if (!key.key_class_resolved || !key.mode.has_value() || *key.mode != diatonic_mode::ionian)
        throw std::invalid_argument("Ionian functional tendency requires a resolved Ionian key-class hypothesis");

    const auto source_degree = infer_diatonic_chord_degree_hypothesis(key, first_chord);
    const auto target_degree = infer_diatonic_chord_degree_hypothesis(key, second_chord);
    const auto transition = infer_harmonic_transition(first_chord, second_chord);

    ionian_functional_tendency_hypothesis result;
    result.first_time = transition.first_time;
    result.second_time = transition.second_time;
    result.source_degree = source_degree.scale_degree;
    result.target_degree = target_degree.scale_degree;
    result.source_quality = source_degree.observed_quality;
    result.target_quality = target_degree.observed_quality;
    result.source_quality_diatonic = source_degree.quality_matches_diatonic_stack;
    result.target_quality_diatonic = target_degree.quality_matches_diatonic_stack;
    result.root_motion_reliable = transition.root_motion_reliable;

    const bool degree_pair_grounded =
        source_degree.scale_degree.has_value() &&
        target_degree.scale_degree.has_value() &&
        source_degree.quality_matches_diatonic_stack &&
        target_degree.quality_matches_diatonic_stack &&
        transition.root_motion_reliable;

    if (degree_pair_grounded) {
        const std::uint8_t source = *source_degree.scale_degree;
        const std::uint8_t target = *target_degree.scale_degree;
        if (ionian_predominant_degree(source) && ionian_dominant_degree(target)) {
            result.kind = ionian_functional_tendency_kind::predominant_progression_candidate;
            result.candidate_resolved = true;
        } else if (ionian_dominant_degree(source) && target == 1) {
            result.kind = ionian_functional_tendency_kind::dominant_resolution_candidate;
            result.candidate_resolved = true;
        }
    }

    if (!result.candidate_resolved)
        return result;

    double evidence_confidence = std::min(
        {key.confidence, source_degree.confidence, target_degree.confidence,
         transition.confidence});
    double confidence_ceiling = ionian_function_degree_transition_ceiling;

    if (voices.has_value()) {
        const auto& value = *voices;
        if (!same_function_transition_time(value.first_time, transition.first_time) ||
            !same_function_transition_time(value.second_time, transition.second_time)) {
            throw std::invalid_argument("functional-tendency voice leading must describe the same harmonic transition");
        }
        result.voice_leading_supplied = true;
        result.voice_leading_identity_grounded = value.all_correspondence_identity_grounded;
        evidence_confidence = std::min(evidence_confidence, value.confidence);
        confidence_ceiling = value.all_correspondence_identity_grounded
            ? ionian_function_identity_voice_ceiling
            : ionian_function_inferred_voice_ceiling;
    }

    if (bass.has_value()) {
        if (!voices.has_value())
            throw std::invalid_argument("bass functional evidence requires its voice-leading witness");
        const auto& value = *bass;
        if (!same_function_transition_time(value.first_time, transition.first_time) ||
            !same_function_transition_time(value.second_time, transition.second_time)) {
            throw std::invalid_argument("functional-tendency bass evidence must describe the same harmonic transition");
        }
        result.bass_evidence_supplied = true;
        result.bass_identity_grounded = value.bass_identity_grounded;
        evidence_confidence = std::min(evidence_confidence, value.confidence);
        // Bass interaction is derived from voice leading, so it can constrain
        // confidence but cannot create a new independent confidence tier.
    }

    if (arrival.has_value()) {
        const auto& value = *arrival;
        if (!same_function_transition_time(value.arrival_time, transition.second_time) ||
            value.root_motion_semitones != transition.directed_root_motion_semitones ||
            value.root_interval_class != transition.root_interval_class ||
            !value.harmonic_root_motion_reliable) {
            throw std::invalid_argument("functional-tendency arrival must describe the same reliable harmonic transition");
        }
        if (value.voice_leading_grounded && !voices.has_value())
            throw std::invalid_argument("voice-grounded arrival requires the corresponding voice-leading witness");
        result.phrase_arrival_supplied = true;
        result.phrase_arrival_cross_part_grounded = value.cross_part_phrase_grounded;
        evidence_confidence = std::min(evidence_confidence, value.confidence);
        if (value.cross_part_phrase_grounded) {
            confidence_ceiling = std::max(
                confidence_ceiling,
                ionian_function_phrase_arrival_ceiling);
        }
    }

    result.confidence = std::min(evidence_confidence, confidence_ceiling);

    // The candidate says what tendency the observed progression is compatible
    // with inside this narrow theory scope. It still does not establish a Roman
    // numeral, secondary function, pivot role, or cadence class.
    result.tonal_function_established = false;
    result.roman_numeral_named = false;
    return result;
}

inline node_id add_ionian_functional_tendency_hypothesis(
    musical_execution_graph& graph,
    const ionian_functional_tendency_hypothesis& hypothesis,
    node_id first_chord_node,
    node_id second_chord_node) {
    if (!hypothesis.candidate_resolved)
        throw std::invalid_argument("only a resolved functional-tendency candidate may be materialized");
    if (graph.find_node(first_chord_node) == nullptr ||
        graph.find_node(second_chord_node) == nullptr) {
        throw std::invalid_argument("functional tendency requires both materialized chord nodes");
    }

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "Ionian functional-tendency hypothesis";
    relation.active = time_span{hypothesis.first_time, hypothesis.second_time};
    relation.attributes.push_back({
        "identity_scope",
        std::string{"ionian_functional_tendency_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "tendency_kind",
        std::string{to_string(hypothesis.kind)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    if (hypothesis.source_degree.has_value()) {
        relation.attributes.push_back({
            "source_scale_degree",
            static_cast<std::uint64_t>(*hypothesis.source_degree),
            evidence_status::hypothesis,
            hypothesis.confidence,
            "1-based diatonic degree",
        });
    }
    if (hypothesis.target_degree.has_value()) {
        relation.attributes.push_back({
            "target_scale_degree",
            static_cast<std::uint64_t>(*hypothesis.target_degree),
            evidence_status::hypothesis,
            hypothesis.confidence,
            "1-based diatonic degree",
        });
    }
    relation.attributes.push_back({
        "tonal_function_established",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "roman_numeral_named",
        false,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "theory_scope",
        hypothesis.theory_scope,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        "resolved Ionian key + chord-degree pair + harmonic transition with optional voice/bass/phrase evidence",
        std::nullopt,
        "functional tendency candidate only; does not establish Roman numeral, secondary function, pivot status, or cadence class",
    });
    const node_id relation_id = graph.add_node(std::move(relation));

    edge first_support;
    first_support.kind = edge_kind::derived_from;
    first_support.from = first_chord_node;
    first_support.to = relation_id;
    graph.add_edge(std::move(first_support));

    edge second_support;
    second_support.kind = edge_kind::derived_from;
    second_support.from = second_chord_node;
    second_support.to = relation_id;
    graph.add_edge(std::move(second_support));
    return relation_id;
}

} // namespace vgmtooling::model
