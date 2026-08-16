#pragma once

#include "ionian_functional_tendency_hypothesis.h"

#include <algorithm>
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
    bool final_soprano_observed = false;
    bool final_soprano_tonic = false;
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

inline ionian_cadence_class_hypothesis infer_ionian_cadence_class_hypothesis(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& first_chord,
    const tertian_triad_hypothesis& second_chord,
    const cadential_arrival_hypothesis& arrival,
    const std::optional<voice_leading_hypothesis>& voices = std::nullopt,
    const std::optional<bass_harmony_interaction_hypothesis>& bass = std::nullopt) {
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

            if (!second_chord.projection.nearest_steps.empty()) {
                result.final_soprano_observed = true;
                const std::int64_t soprano_pitch_class = positive_mod(
                    second_chord.projection.nearest_steps.back(),
                    12);
                result.final_soprano_tonic =
                    soprano_pitch_class == positive_mod(key.center_pitch_class, 12);

                const double voicing_confidence = std::min(
                    {first_chord.projection.confidence,
                     second_chord.projection.confidence,
                     key.tuning.confidence});
                if (result.source_root_position &&
                    result.target_root_position &&
                    result.final_soprano_tonic) {
                    result.kind = ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate;
                } else {
                    result.kind = ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate;
                }
                result.confidence = std::min(
                    {result.confidence,
                     voicing_confidence,
                     ionian_specific_authentic_cadence_ceiling});
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
        "Ionian functional tendency + cross-part phrase arrival + available inversion/soprano evidence",
        std::nullopt,
        "cadence-class candidate only; this does not establish Roman numeral, global key function, or a universal cadence ontology outside the stated theory scope",
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
