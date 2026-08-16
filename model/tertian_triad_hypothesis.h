#pragma once

#include "tuning_projection.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class tertian_triad_quality : std::uint8_t {
    major = 0,
    minor,
    diminished,
    augmented,
};

enum class triad_inversion : std::uint8_t {
    root_position = 0,
    first,
    second,
    unknown,
};

struct tertian_triad_hypothesis {
    std::int64_t root_pitch_class = 0;
    tertian_triad_quality quality = tertian_triad_quality::major;
    triad_inversion inversion = triad_inversion::unknown;
    double confidence = 0.0;
    bool root_ambiguous = false;
    std::vector<std::int64_t> pitch_classes;
    equal_temperament_pitch_projection projection;
};

constexpr double ambiguous_triad_root_ceiling = 0.60;

inline const char* to_string(tertian_triad_quality quality) noexcept {
    switch (quality) {
    case tertian_triad_quality::major:
        return "major";
    case tertian_triad_quality::minor:
        return "minor";
    case tertian_triad_quality::diminished:
        return "diminished";
    case tertian_triad_quality::augmented:
        return "augmented";
    }
    return "unknown";
}

inline const char* to_string(triad_inversion inversion) noexcept {
    switch (inversion) {
    case triad_inversion::root_position:
        return "root_position";
    case triad_inversion::first:
        return "first_inversion";
    case triad_inversion::second:
        return "second_inversion";
    case triad_inversion::unknown:
        return "unknown";
    }
    return "unknown";
}

inline std::vector<std::int64_t> triad_template(tertian_triad_quality quality) {
    switch (quality) {
    case tertian_triad_quality::major:
        return {0, 4, 7};
    case tertian_triad_quality::minor:
        return {0, 3, 7};
    case tertian_triad_quality::diminished:
        return {0, 3, 6};
    case tertian_triad_quality::augmented:
        return {0, 4, 8};
    }
    return {};
}

inline triad_inversion infer_triad_inversion(
    std::int64_t bass_pitch_class,
    std::int64_t root_pitch_class,
    tertian_triad_quality quality) {
    const auto offsets = triad_template(quality);
    const std::int64_t relative = positive_mod(bass_pitch_class - root_pitch_class, 12);
    if (relative == offsets[0])
        return triad_inversion::root_position;
    if (relative == offsets[1])
        return triad_inversion::first;
    if (relative == offsets[2])
        return triad_inversion::second;
    return triad_inversion::unknown;
}

inline std::vector<tertian_triad_hypothesis> infer_tertian_triad_hypotheses(
    const equal_temperament_pitch_projection& projection) {
    if (projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("tertian triad inference currently requires an explicit 12-TET projection");
    if (projection.nearest_steps.size() < 3)
        return {};

    std::set<std::int64_t> unique_classes;
    for (std::int64_t step : projection.nearest_steps)
        unique_classes.insert(positive_mod(step, 12));
    if (unique_classes.size() != 3)
        return {};

    std::vector<std::int64_t> classes(unique_classes.begin(), unique_classes.end());
    const std::int64_t bass_class = positive_mod(
        *std::min_element(projection.nearest_steps.begin(), projection.nearest_steps.end()),
        12);

    std::vector<tertian_triad_hypothesis> results;
    for (std::int64_t root = 0; root < 12; ++root) {
        for (tertian_triad_quality quality : {
                 tertian_triad_quality::major,
                 tertian_triad_quality::minor,
                 tertian_triad_quality::diminished,
                 tertian_triad_quality::augmented,
             }) {
            std::set<std::int64_t> candidate;
            for (std::int64_t offset : triad_template(quality))
                candidate.insert(positive_mod(root + offset, 12));
            if (candidate != unique_classes)
                continue;

            tertian_triad_hypothesis hypothesis;
            hypothesis.root_pitch_class = root;
            hypothesis.quality = quality;
            hypothesis.inversion = infer_triad_inversion(bass_class, root, quality);
            hypothesis.confidence = projection.confidence;
            hypothesis.pitch_classes = classes;
            hypothesis.projection = projection;
            results.push_back(std::move(hypothesis));
        }
    }

    if (results.size() > 1) {
        for (auto& hypothesis : results) {
            hypothesis.root_ambiguous = true;
            hypothesis.confidence = std::min(
                hypothesis.confidence,
                ambiguous_triad_root_ceiling);
        }
    }
    return results;
}

inline node_id add_tertian_triad_hypothesis(
    musical_execution_graph& graph,
    const tertian_triad_hypothesis& hypothesis) {
    if (hypothesis.projection.source_verticality.source_nodes.empty())
        throw std::invalid_argument("tertian triad hypothesis has no verticality evidence");
    for (node_id source_id : hypothesis.projection.source_verticality.source_nodes) {
        if (graph.find_node(source_id) == nullptr)
            throw std::invalid_argument("tertian triad hypothesis references an unknown pitch source");
    }

    node chord;
    chord.kind = node_kind::pattern;
    chord.layer = semantic_layer::musical_structure;
    chord.flow = flow_kind::value;
    chord.label = "tertian triad hypothesis";
    chord.active = time_span{
        hypothesis.projection.source_verticality.observation_time,
        std::nullopt,
    };
    chord.attributes.push_back({
        "identity_scope",
        std::string{"tertian_triad_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    chord.attributes.push_back({
        "root_pitch_class",
        hypothesis.root_pitch_class,
        evidence_status::hypothesis,
        hypothesis.confidence,
        "12-TET pitch class",
    });
    chord.attributes.push_back({
        "quality",
        std::string{to_string(hypothesis.quality)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    chord.attributes.push_back({
        "inversion",
        std::string{to_string(hypothesis.inversion)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    chord.attributes.push_back({
        "root_ambiguous",
        hypothesis.root_ambiguous,
        evidence_status::derived,
        1.0,
        "",
    });
    chord.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        hypothesis.projection.tuning.source,
        std::nullopt,
        "tertian triad quality/root candidate under explicit 12-TET projection; this does not establish key, Roman-numeral function, cadence, or enharmonic spelling",
    });
    const node_id chord_id = graph.add_node(std::move(chord));

    for (node_id source_id : hypothesis.projection.source_verticality.source_nodes) {
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = source_id;
        support.to = chord_id;
        support.attributes.push_back({
            "support_role",
            std::string{"projected_absolute_pitch"},
            evidence_status::derived,
            hypothesis.confidence,
            "",
        });
        graph.add_edge(std::move(support));
    }
    return chord_id;
}

} // namespace vgmtooling::model
