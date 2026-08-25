#pragma once

#include "cadential_melodic_arrival_evidence.h"
#include "ionian_functional_tendency_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

enum class ionian_cadence_candidate_kind : std::uint8_t {
    unresolved = 0,
    authentic_cadence_candidate,
    perfect_authentic_cadence_candidate,
    imperfect_authentic_cadence_candidate,
    predominant_half_cadence_candidate,
    leading_tone_resolution_candidate,
};

struct ionian_cadence_class_hypothesis {
    ionian_cadence_candidate_kind kind = ionian_cadence_candidate_kind::unresolved;
    time_coordinate arrival_time{};
    ionian_functional_tendency_kind functional_tendency =
        ionian_functional_tendency_kind::unresolved;
    std::optional<std::uint8_t> source_degree{};
    std::optional<std::uint8_t> target_degree{};
    bool phrase_cross_part_grounded = false;
    bool source_root_position = false;
    bool target_root_position = false;
    bool final_melodic_arrival_grounded = false;
    node_id final_melodic_part_id = 0;
    std::optional<std::int64_t> final_melodic_pitch_class{};
    bool final_melodic_tonic = false;
    bool cadence_candidate_resolved = false;
    bool cadence_class_established = false;
    bool roman_numeral_named = false;
    double confidence = 0.0;
    std::string theory_scope = "12-TET Ionian common-practice cadence candidate";
};

constexpr double ionian_generic_authentic_cadence_ceiling = 0.82;
constexpr double ionian_specific_authentic_cadence_ceiling = 0.82;
constexpr double ionian_predominant_half_cadence_ceiling = 0.78;
constexpr double ionian_leading_tone_resolution_ceiling = 0.74;

inline const char* to_string(ionian_cadence_candidate_kind kind) noexcept {
    switch (kind) {
    case ionian_cadence_candidate_kind::unresolved:
        return "unresolved";
    case ionian_cadence_candidate_kind::authentic_cadence_candidate:
        return "authentic_cadence_candidate";
    case ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate:
        return "perfect_authentic_cadence_candidate";
    case ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate:
        return "imperfect_authentic_cadence_candidate";
    case ionian_cadence_candidate_kind::predominant_half_cadence_candidate:
        return "predominant_half_cadence_candidate";
    case ionian_cadence_candidate_kind::leading_tone_resolution_candidate:
        return "leading_tone_resolution_candidate";
    }
    return "unknown";
}

inline void validate_cadential_melodic_arrival_for_chord(
    const cadential_melodic_arrival_evidence& melodic,
    const tertian_triad_hypothesis& chord) {
    if (melodic.part_id == 0 || !melodic.melodic_role_grounded ||
        !std::isfinite(melodic.confidence) ||
        melodic.confidence < 0.0 || melodic.confidence > 1.0) {
        throw std::invalid_argument("cadence melodic arrival is not grounded");
    }
    const auto chord_time = chord.projection.source_verticality.observation_time;
    if (!same_function_transition_time(melodic.arrival_time, chord_time) ||
        melodic.pitch_role != chord.projection.source_verticality.role) {
        throw std::invalid_argument(
            "cadence melodic arrival must describe the same projected chord observation");
    }
    const auto& parts = chord.projection.source_verticality.part_ids;
    const auto& steps = chord.projection.nearest_steps;
    if (parts.size() != steps.size())
        throw std::invalid_argument(
            "cadence melodic validation requires one persistent-part id per projected pitch");

    std::optional<std::size_t> match;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (parts[index] != melodic.part_id)
            continue;
        if (match.has_value())
            throw std::invalid_argument(
                "cadence melodic arrival part is ambiguous in the final sonority");
        match = index;
    }
    if (!match.has_value() ||
        steps[*match] != melodic.projected_step ||
        positive_mod(steps[*match], 12) != positive_mod(melodic.pitch_class, 12)) {
        throw std::invalid_argument(
            "cadence melodic arrival pitch does not match its persistent part in the final sonority");
    }
}

inline ionian_cadence_class_hypothesis infer_ionian_cadence_class_hypothesis(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& first_chord,
    const tertian_triad_hypothesis& second_chord,
    const cadential_arrival_hypothesis& arrival,
    const std::optional<voice_leading_hypothesis>& voices = std::nullopt,
    const std::optional<bass_harmony_interaction_hypothesis>& bass = std::nullopt,
    const std::optional<cadential_melodic_arrival_evidence>& melodic = std::nullopt) {
    const auto tendency = infer_ionian_functional_tendency(
        key,
        first_chord,
        second_chord,
        voices,
        bass,
        arrival);

    ionian_cadence_class_hypothesis result;
    result.arrival_time = arrival.arrival_time;
    result.functional_tendency = tendency.kind;
    result.source_degree = tendency.source_degree;
    result.target_degree = tendency.target_degree;
    result.phrase_cross_part_grounded = arrival.cross_part_phrase_grounded;
    result.source_root_position = first_chord.inversion == triad_inversion::root_position;
    result.target_root_position = second_chord.inversion == triad_inversion::root_position;

    if (melodic.has_value()) {
        validate_cadential_melodic_arrival_for_chord(*melodic, second_chord);
        result.final_melodic_arrival_grounded = true;
        result.final_melodic_part_id = melodic->part_id;
        result.final_melodic_pitch_class = positive_mod(melodic->pitch_class, 12);
        result.final_melodic_tonic =
            *result.final_melodic_pitch_class == positive_mod(key.center_pitch_class, 12);
    }

    if (!arrival.cross_part_phrase_grounded || !tendency.candidate_resolved)
        return result;

    if (tendency.kind == ionian_functional_tendency_kind::dominant_resolution_candidate &&
        tendency.source_degree.has_value() && tendency.target_degree.has_value()) {
        const std::uint8_t source = *tendency.source_degree;
        const std::uint8_t target = *tendency.target_degree;

        if (source == 5 && target == 1) {
            result.kind = ionian_cadence_candidate_kind::authentic_cadence_candidate;
            result.cadence_candidate_resolved = true;
            result.confidence = std::min(
                tendency.confidence,
                ionian_generic_authentic_cadence_ceiling);

            // PAC/IAC refinement needs an independently grounded melodic part.
            // The highest sounding pitch is deliberately not used as a proxy for
            // soprano/foreground identity in reconstructed game-music textures.
            if (melodic.has_value()) {
                if (result.source_root_position &&
                    result.target_root_position &&
                    result.final_melodic_tonic) {
                    result.kind = ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate;
                } else {
                    result.kind = ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate;
                }
                result.confidence = std::min({
                    result.confidence,
                    melodic->confidence,
                    ionian_specific_authentic_cadence_ceiling,
                });
            }
        } else if (source == 7 && target == 1) {
            // This is cadence-like resolution pressure, but it is not relabeled
            // as an authentic cadence because the source harmony is vii°, not V.
            result.kind = ionian_cadence_candidate_kind::leading_tone_resolution_candidate;
            result.cadence_candidate_resolved = true;
            result.confidence = std::min(
                tendency.confidence,
                ionian_leading_tone_resolution_ceiling);
        }
    } else if (
        tendency.kind == ionian_functional_tendency_kind::predominant_progression_candidate &&
        tendency.target_degree.has_value() &&
        *tendency.target_degree == 5) {
        // Deliberately narrow: ii/IV -> V at a grounded phrase arrival is a
        // half-cadence candidate. Other approaches to V can be added later.
        result.kind = ionian_cadence_candidate_kind::predominant_half_cadence_candidate;
        result.cadence_candidate_resolved = true;
        result.confidence = std::min(
            tendency.confidence,
            ionian_predominant_half_cadence_ceiling);
    }

    result.cadence_class_established = false;
    result.roman_numeral_named = false;
    return result;
}

inline node_id add_ionian_cadence_class_hypothesis(
    musical_execution_graph& graph,
    const ionian_cadence_class_hypothesis& hypothesis,
    node_id first_chord_node,
    node_id second_chord_node) {
    if (!hypothesis.cadence_candidate_resolved)
        throw std::invalid_argument("only a resolved cadence candidate may be materialized");
    if (graph.find_node(first_chord_node) == nullptr ||
        graph.find_node(second_chord_node) == nullptr) {
        throw std::invalid_argument("cadence candidate requires both materialized chord nodes");
    }

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "Ionian cadence-class hypothesis";
    relation.active = time_span{hypothesis.arrival_time, hypothesis.arrival_time};
    relation.attributes.push_back({
        "identity_scope",
        std::string{"ionian_cadence_class_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "cadence_candidate_kind",
        std::string{to_string(hypothesis.kind)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "phrase_cross_part_grounded",
        hypothesis.phrase_cross_part_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "final_melodic_arrival_grounded",
        hypothesis.final_melodic_arrival_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    if (hypothesis.final_melodic_arrival_grounded) {
        relation.attributes.push_back({
            "final_melodic_part_id",
            static_cast<std::uint64_t>(hypothesis.final_melodic_part_id),
            evidence_status::derived,
            hypothesis.confidence,
            "node_id",
        });
    }
    relation.attributes.push_back({
        "cadence_class_established",
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
        "Ionian functional tendency + cross-part phrase arrival + inversion evidence + optional persistent melodic-arrival evidence",
        std::nullopt,
        "cadence-class candidate only; PAC/IAC refinement requires a separately grounded melodic-foreground part and does not use highest pitch as a voice-identity shortcut",
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
