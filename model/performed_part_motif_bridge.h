#pragma once

#include "part_motif_profile.h"
#include "persistent_part_performance_trajectory.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

inline std::optional<part_gesture_performance_shape>
resolved_part_gesture_performance_shape(
    const pitch_motion_articulation_hypothesis& articulation) {
    if (articulation.kind == pitch_motion_articulation_kind::in_episode_pitch_change_unresolved ||
        articulation.kind == pitch_motion_articulation_kind::rearticulation_boundary)
        return std::nullopt;
    if (!std::isfinite(articulation.pitch_range_semitones) ||
        articulation.pitch_range_semitones < 0.0 ||
        !std::isfinite(articulation.net_motion_semitones) ||
        !std::isfinite(articulation.confidence) ||
        articulation.confidence < 0.0 || articulation.confidence > 1.0) {
        throw std::invalid_argument("resolved performance shape carries invalid motion evidence");
    }
    return part_gesture_performance_shape{
        articulation.kind,
        articulation.pitch_range_semitones,
        articulation.net_motion_semitones,
        articulation.direction_changes,
        articulation.confidence,
    };
}

inline std::vector<part_gesture_observation>
make_performed_part_gesture_observations(
    const persistent_part_performance_trajectory& performance,
    node_id part_id,
    evidence_status part_status,
    double part_confidence) {
    if (part_id == 0)
        throw std::invalid_argument("performed motif bridge requires a persistent-part id");
    if (!std::isfinite(part_confidence) || part_confidence < 0.0 || part_confidence > 1.0)
        throw std::invalid_argument("performed motif bridge requires bounded part confidence");
    if (performance.segments.size() != performance.identity.subject_nodes.size() ||
        performance.segments.empty())
        throw std::invalid_argument("performed motif bridge requires one segment per identity episode");

    std::vector<part_gesture_observation> result;
    result.reserve(performance.segments.size());
    for (std::size_t index = 0; index < performance.segments.size(); ++index) {
        const auto& segment = performance.segments[index];
        if (segment.physical_episode_id != performance.identity.subject_nodes[index] ||
            segment.samples.empty()) {
            throw std::invalid_argument("performed motif segment no longer matches persistent identity");
        }
        const auto& onset = segment.samples.front();
        if (onset.source_node == 0 || onset.physical_episode_id != segment.physical_episode_id ||
            onset.pitch_basis.empty() || onset.interval_semantics.empty() ||
            !std::isfinite(onset.log2_pitch_coordinate)) {
            throw std::invalid_argument("performed motif onset lacks source-backed pitch semantics");
        }

        const auto shape = resolved_part_gesture_performance_shape(segment.articulation);
        double confidence = std::min({
            performance.confidence,
            part_confidence,
            segment.articulation.confidence,
        });
        if (shape.has_value())
            confidence = std::min(confidence, shape->confidence);

        part_gesture_observation observation{
            onset.source_node,
            part_id,
            onset.time,
            onset.log2_pitch_coordinate,
            onset.pitch_basis,
            onset.interval_semantics,
            part_status,
            confidence,
        };
        observation.performance_shape = shape;
        result.push_back(std::move(observation));
    }
    return result;
}

} // namespace vgmtooling::model
