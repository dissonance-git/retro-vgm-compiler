#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class musical_pitch_role : std::uint8_t {
    programmed = 0,
    performed,
    heard,
};

struct absolute_musical_pitch_observation {
    node_id source_node = 0;
    node_id part_id = 0;
    time_span active{};
    double frequency_hz = 0.0;
    musical_pitch_role role = musical_pitch_role::programmed;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
};

struct harmonic_verticality {
    time_coordinate observation_time{};
    musical_pitch_role role = musical_pitch_role::programmed;
    std::vector<node_id> source_nodes;
    std::vector<node_id> part_ids;
    std::vector<double> frequencies_hz;
    std::vector<double> intervals_above_lowest_octaves;
    double confidence = 0.0;
};

inline const char* to_string(musical_pitch_role role) noexcept {
    switch (role) {
    case musical_pitch_role::programmed:
        return "programmed";
    case musical_pitch_role::performed:
        return "performed";
    case musical_pitch_role::heard:
        return "heard";
    }
    return "unknown";
}

inline void validate_absolute_musical_pitch_observation(
    const absolute_musical_pitch_observation& observation) {
    if (observation.source_node == 0)
        throw std::invalid_argument("absolute musical pitch requires a source node");
    if (observation.part_id == 0)
        throw std::invalid_argument("absolute musical pitch requires a persistent-part id");
    if (!std::isfinite(observation.frequency_hz) || observation.frequency_hz <= 0.0)
        throw std::invalid_argument("absolute musical pitch frequency must be finite and positive");
    if (observation.confidence < 0.0 || observation.confidence > 1.0)
        throw std::invalid_argument("absolute musical pitch confidence must be in [0, 1]");
    if (observation.source.empty())
        throw std::invalid_argument("absolute musical pitch requires provenance source");
}

inline bool time_coordinate_inside_span(
    const time_coordinate& coordinate,
    const time_span& span) noexcept {
    if (coordinate.domain != span.start.domain ||
        coordinate.tick_rate != span.start.tick_rate ||
        coordinate.loop_iteration != span.start.loop_iteration) {
        return false;
    }
    if (coordinate.tick < span.start.tick)
        return false;
    if (!span.end.has_value())
        return true;
    return coordinate.tick < span.end->tick;
}

inline harmonic_verticality make_harmonic_verticality(
    time_coordinate observation_time,
    std::vector<absolute_musical_pitch_observation> observations) {
    if (observations.size() < 2)
        throw std::invalid_argument("harmonic verticality requires at least two pitch observations");

    const musical_pitch_role role = observations.front().role;
    std::vector<absolute_musical_pitch_observation> active;
    active.reserve(observations.size());
    for (const auto& observation : observations) {
        validate_absolute_musical_pitch_observation(observation);
        if (observation.role != role)
            throw std::invalid_argument("harmonic verticality cannot silently mix programmed, performed, and heard pitch roles");
        if (time_coordinate_inside_span(observation_time, observation.active))
            active.push_back(observation);
    }
    if (active.size() < 2)
        throw std::invalid_argument("fewer than two supplied musical pitches are active at the observation time");

    std::sort(active.begin(), active.end(), [](const auto& first, const auto& second) {
        if (first.frequency_hz != second.frequency_hz)
            return first.frequency_hz < second.frequency_hz;
        return first.source_node < second.source_node;
    });

    harmonic_verticality result;
    result.observation_time = observation_time;
    result.role = role;
    result.confidence = 1.0;
    result.source_nodes.reserve(active.size());
    result.part_ids.reserve(active.size());
    result.frequencies_hz.reserve(active.size());
    result.intervals_above_lowest_octaves.reserve(active.size());

    const double lowest = active.front().frequency_hz;
    for (const auto& observation : active) {
        result.source_nodes.push_back(observation.source_node);
        result.part_ids.push_back(observation.part_id);
        result.frequencies_hz.push_back(observation.frequency_hz);
        result.intervals_above_lowest_octaves.push_back(
            std::log2(observation.frequency_hz / lowest));
        result.confidence = std::min(result.confidence, observation.confidence);
    }
    return result;
}

inline node_id add_harmonic_verticality(
    musical_execution_graph& graph,
    const harmonic_verticality& verticality) {
    if (verticality.source_nodes.size() < 2 ||
        verticality.source_nodes.size() != verticality.part_ids.size() ||
        verticality.source_nodes.size() != verticality.frequencies_hz.size() ||
        verticality.source_nodes.size() != verticality.intervals_above_lowest_octaves.size()) {
        throw std::invalid_argument("harmonic verticality is incomplete");
    }
    for (std::size_t index = 0; index < verticality.source_nodes.size(); ++index) {
        if (graph.find_node(verticality.source_nodes[index]) == nullptr)
            throw std::invalid_argument("harmonic verticality references an unknown source node");
        const node* part = graph.find_node(verticality.part_ids[index]);
        if (part == nullptr || part->kind != node_kind::part)
            throw std::invalid_argument("harmonic verticality references an unknown persistent part");
    }

    node collection;
    collection.kind = node_kind::pattern;
    collection.layer = semantic_layer::musical_structure;
    collection.flow = flow_kind::value;
    collection.label = "simultaneous musical pitch collection";
    collection.active = time_span{verticality.observation_time, std::nullopt};
    collection.attributes.push_back({
        "identity_scope",
        std::string{"harmonic_verticality"},
        evidence_status::derived,
        verticality.confidence,
        "",
    });
    collection.attributes.push_back({
        "pitch_role",
        std::string{to_string(verticality.role)},
        evidence_status::derived,
        verticality.confidence,
        "",
    });
    collection.attributes.push_back({
        "pitch_count",
        static_cast<std::uint64_t>(verticality.frequencies_hz.size()),
        evidence_status::derived,
        1.0,
        "pitches",
    });
    collection.provenance.push_back({
        evidence_status::derived,
        verticality.confidence,
        "harmonic-verticality-analysis",
        std::nullopt,
        "simultaneous absolute musical pitches; this node does not claim chord spelling, root, function, cadence, or key",
    });
    const node_id collection_id = graph.add_node(std::move(collection));

    for (std::size_t index = 0; index < verticality.source_nodes.size(); ++index) {
        edge relation;
        relation.kind = edge_kind::derived_from;
        relation.from = verticality.source_nodes[index];
        relation.to = collection_id;
        relation.attributes.push_back({
            "support_role",
            std::string{"absolute_musical_pitch"},
            evidence_status::derived,
            verticality.confidence,
            "",
        });
        relation.attributes.push_back({
            "persistent_part_id",
            static_cast<std::uint64_t>(verticality.part_ids[index]),
            evidence_status::derived,
            1.0,
            "node_id",
        });
        relation.attributes.push_back({
            "frequency_hz",
            verticality.frequencies_hz[index],
            evidence_status::derived,
            verticality.confidence,
            "Hz",
        });
        relation.attributes.push_back({
            "interval_above_lowest_octaves",
            verticality.intervals_above_lowest_octaves[index],
            evidence_status::derived,
            verticality.confidence,
            "octaves",
        });
        graph.add_edge(std::move(relation));
    }
    return collection_id;
}

} // namespace vgmtooling::model
