#pragma once

#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct harmonic_transition_hypothesis {
    time_coordinate first_time{};
    time_coordinate second_time{};
    std::int64_t first_root_pitch_class = 0;
    std::int64_t second_root_pitch_class = 0;
    std::int64_t directed_root_motion_semitones = 0;
    std::int64_t root_interval_class = 0;
    tertian_triad_quality first_quality = tertian_triad_quality::major;
    tertian_triad_quality second_quality = tertian_triad_quality::major;
    bool quality_changed = false;
    std::size_t common_pitch_classes = 0;
    bool root_motion_reliable = true;
    double confidence = 0.0;
};

constexpr double ambiguous_root_transition_ceiling = 0.60;

inline std::set<std::int64_t> triad_pitch_class_set(
    const tertian_triad_hypothesis& chord) {
    return std::set<std::int64_t>(chord.pitch_classes.begin(), chord.pitch_classes.end());
}

inline harmonic_transition_hypothesis infer_harmonic_transition(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second) {
    const time_coordinate first_time = first.projection.source_verticality.observation_time;
    const time_coordinate second_time = second.projection.source_verticality.observation_time;
    if (first_time.domain != second_time.domain ||
        first_time.tick_rate != second_time.tick_rate ||
        first_time.loop_iteration != second_time.loop_iteration) {
        throw std::invalid_argument("harmonic transition requires one compatible local time basis");
    }
    if (second_time.tick <= first_time.tick)
        throw std::invalid_argument("harmonic transition second chord must follow the first");

    harmonic_transition_hypothesis result;
    result.first_time = first_time;
    result.second_time = second_time;
    result.first_root_pitch_class = first.root_pitch_class;
    result.second_root_pitch_class = second.root_pitch_class;
    result.directed_root_motion_semitones = positive_mod(
        second.root_pitch_class - first.root_pitch_class,
        12);
    result.root_interval_class = std::min(
        result.directed_root_motion_semitones,
        12 - result.directed_root_motion_semitones);
    result.first_quality = first.quality;
    result.second_quality = second.quality;
    result.quality_changed = first.quality != second.quality;
    result.root_motion_reliable = !first.root_ambiguous && !second.root_ambiguous;

    const auto first_classes = triad_pitch_class_set(first);
    const auto second_classes = triad_pitch_class_set(second);
    for (std::int64_t pitch_class : first_classes)
        result.common_pitch_classes += second_classes.count(pitch_class) != 0 ? 1u : 0u;

    result.confidence = std::min(first.confidence, second.confidence);
    if (!result.root_motion_reliable)
        result.confidence = std::min(result.confidence, ambiguous_root_transition_ceiling);
    return result;
}

inline bool is_tertian_triad_node(const node& value) noexcept {
    if (value.kind != node_kind::pattern || value.layer != semantic_layer::musical_structure)
        return false;
    for (const auto& item : value.attributes) {
        if (item.key != "identity_scope")
            continue;
        const auto* text = std::get_if<std::string>(&item.value);
        return text != nullptr && *text == "tertian_triad_hypothesis";
    }
    return false;
}

inline edge_id add_harmonic_transition_hypothesis(
    musical_execution_graph& graph,
    node_id first_chord,
    node_id second_chord,
    const harmonic_transition_hypothesis& transition) {
    const node* first = graph.find_node(first_chord);
    const node* second = graph.find_node(second_chord);
    if (first == nullptr || !is_tertian_triad_node(*first) ||
        second == nullptr || !is_tertian_triad_node(*second)) {
        throw std::invalid_argument("harmonic transition requires materialized tertian-triad nodes");
    }

    edge relation;
    relation.kind = edge_kind::transforms;
    relation.from = first_chord;
    relation.to = second_chord;
    relation.attributes.push_back({
        "identity_scope",
        std::string{"harmonic_transition_hypothesis"},
        evidence_status::hypothesis,
        transition.confidence,
        "",
    });
    relation.attributes.push_back({
        "directed_root_motion_semitones",
        transition.directed_root_motion_semitones,
        evidence_status::hypothesis,
        transition.confidence,
        "12-TET semitones",
    });
    relation.attributes.push_back({
        "root_interval_class",
        transition.root_interval_class,
        evidence_status::hypothesis,
        transition.confidence,
        "12-TET interval class",
    });
    relation.attributes.push_back({
        "quality_changed",
        transition.quality_changed,
        evidence_status::derived,
        transition.confidence,
        "",
    });
    relation.attributes.push_back({
        "common_pitch_classes",
        static_cast<std::uint64_t>(transition.common_pitch_classes),
        evidence_status::derived,
        transition.confidence,
        "pitch classes",
    });
    relation.attributes.push_back({
        "root_motion_reliable",
        transition.root_motion_reliable,
        evidence_status::derived,
        transition.confidence,
        "",
    });
    relation.provenance.push_back({
        evidence_status::hypothesis,
        transition.confidence,
        "harmonic-transition-analysis",
        std::nullopt,
        "relation between explicit triad hypotheses; does not establish key, Roman-numeral function, cadence, or tonal destination",
    });
    return graph.add_edge(std::move(relation));
}

} // namespace vgmtooling::model
