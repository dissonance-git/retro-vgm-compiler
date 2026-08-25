#pragma once

#include "persistent_part_trajectory.h"
#include "pitch_motion_articulation.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct persistent_part_performance_segment {
    node_id physical_episode_id = 0;
    std::vector<pitch_motion_sample> samples;
    pitch_motion_articulation_hypothesis articulation;
};

struct persistent_part_performance_trajectory {
    persistent_part_trajectory identity;
    std::vector<persistent_part_performance_segment> segments;
    std::vector<pitch_motion_articulation_hypothesis> rearticulation_boundaries;
    double confidence = 0.0;
};

inline persistent_part_performance_segment make_persistent_part_performance_segment(
    std::vector<pitch_motion_sample> samples,
    double confidence,
    pitch_motion_analysis_policy policy = {}) {
    if (samples.empty())
        throw std::invalid_argument("persistent-part performance segment requires pitch samples");

    persistent_part_performance_segment result;
    result.physical_episode_id = samples.front().physical_episode_id;
    result.articulation = analyze_in_episode_pitch_motion(samples, confidence, policy);
    result.samples = std::move(samples);
    return result;
}

inline persistent_part_performance_trajectory make_persistent_part_performance_trajectory(
    persistent_part_trajectory identity,
    std::vector<persistent_part_performance_segment> segments,
    std::vector<pitch_motion_articulation_hypothesis> rearticulation_boundaries) {
    if (identity.subject_nodes.size() < 2 || identity.transitions.empty())
        throw std::invalid_argument("persistent-part performance trajectory requires persistent identity");
    if (segments.size() != identity.subject_nodes.size())
        throw std::invalid_argument("persistent-part performance trajectory requires one segment per identity episode");
    if (rearticulation_boundaries.size() + 1 != identity.subject_nodes.size())
        throw std::invalid_argument("persistent-part performance trajectory requires one boundary between adjacent episodes");
    if (identity.confidence < 0.0 || identity.confidence > 1.0)
        throw std::invalid_argument("persistent-part performance identity confidence must be in [0, 1]");

    persistent_part_performance_trajectory result;
    result.identity = std::move(identity);
    result.segments = std::move(segments);
    result.rearticulation_boundaries = std::move(rearticulation_boundaries);
    result.confidence = result.identity.confidence;

    for (std::size_t index = 0; index < result.segments.size(); ++index) {
        const node_id expected_episode = result.identity.subject_nodes[index];
        const auto& segment = result.segments[index];
        const auto& articulation = segment.articulation;
        if (segment.physical_episode_id != expected_episode ||
            articulation.physical_episode_id != expected_episode) {
            throw std::invalid_argument("persistent-part performance segment does not match identity order");
        }
        if (segment.samples.empty())
            throw std::invalid_argument("persistent-part performance segment lost its pitch trajectory");
        if (articulation.kind == pitch_motion_articulation_kind::rearticulation_boundary ||
            articulation.rearticulation_supported ||
            articulation.next_physical_episode_id != 0) {
            throw std::invalid_argument("persistent-part performance segment cannot impersonate a rearticulation boundary");
        }
        if (articulation.confidence < 0.0 || articulation.confidence > 1.0)
            throw std::invalid_argument("persistent-part performance confidence must be in [0, 1]");
        if (articulation.source_nodes.size() != segment.samples.size())
            throw std::invalid_argument("persistent-part performance segment lost source-node correspondence");

        for (std::size_t sample_index = 0; sample_index < segment.samples.size(); ++sample_index) {
            const auto& sample = segment.samples[sample_index];
            if (sample.physical_episode_id != expected_episode ||
                sample.source_node != articulation.source_nodes[sample_index]) {
                throw std::invalid_argument("persistent-part performance samples no longer match their articulation evidence");
            }
        }
        result.confidence = std::min(result.confidence, articulation.confidence);
    }

    for (std::size_t index = 0; index < result.rearticulation_boundaries.size(); ++index) {
        const auto& boundary = result.rearticulation_boundaries[index];
        if (boundary.kind != pitch_motion_articulation_kind::rearticulation_boundary ||
            !boundary.rearticulation_supported) {
            throw std::invalid_argument("persistent-part performance boundary lacks rearticulation evidence");
        }
        if (boundary.physical_episode_id != result.identity.subject_nodes[index] ||
            boundary.next_physical_episode_id != result.identity.subject_nodes[index + 1]) {
            throw std::invalid_argument("persistent-part performance boundary does not match identity adjacency");
        }
        if (boundary.confidence < 0.0 || boundary.confidence > 1.0)
            throw std::invalid_argument("persistent-part rearticulation confidence must be in [0, 1]");
        result.confidence = std::min(result.confidence, boundary.confidence);
    }

    return result;
}

} // namespace vgmtooling::model
