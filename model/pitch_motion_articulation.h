#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace vgmtooling::model {

enum class pitch_motion_articulation_kind : std::uint8_t {
    steady_pitch = 0,
    periodic_modulation_candidate,
    glide_candidate,
    in_episode_pitch_change_unresolved,
    rearticulation_boundary,
};

struct pitch_motion_sample {
    node_id source_node = 0;
    node_id physical_episode_id = 0;
    time_coordinate time{};
    double log2_pitch_coordinate = 0.0;
    std::string pitch_basis;
    std::string interval_semantics;
};

struct pitch_motion_articulation_hypothesis {
    pitch_motion_articulation_kind kind =
        pitch_motion_articulation_kind::in_episode_pitch_change_unresolved;
    node_id physical_episode_id = 0;
    node_id next_physical_episode_id = 0;
    std::vector<node_id> source_nodes;
    std::vector<double> step_semitones;
    double pitch_range_semitones = 0.0;
    double net_motion_semitones = 0.0;
    std::size_t direction_changes = 0;
    bool rearticulation_supported = false;
    double confidence = 0.0;
};

struct pitch_motion_analysis_policy {
    double stationary_tolerance_semitones = 0.05;
    double modulation_range_ceiling_semitones = 2.0;
    double glide_minimum_net_motion_semitones = 1.0;
    std::size_t modulation_minimum_direction_changes = 2;
    std::size_t modulation_minimum_samples = 5;
    std::size_t glide_minimum_samples = 4;
};

inline const char* to_string(pitch_motion_articulation_kind kind) noexcept {
    switch (kind) {
    case pitch_motion_articulation_kind::steady_pitch:
        return "steady_pitch";
    case pitch_motion_articulation_kind::periodic_modulation_candidate:
        return "periodic_modulation_candidate";
    case pitch_motion_articulation_kind::glide_candidate:
        return "glide_candidate";
    case pitch_motion_articulation_kind::in_episode_pitch_change_unresolved:
        return "in_episode_pitch_change_unresolved";
    case pitch_motion_articulation_kind::rearticulation_boundary:
        return "rearticulation_boundary";
    }
    return "unknown";
}

inline void validate_pitch_motion_policy(const pitch_motion_analysis_policy& policy) {
    if (!std::isfinite(policy.stationary_tolerance_semitones) ||
        policy.stationary_tolerance_semitones < 0.0 ||
        !std::isfinite(policy.modulation_range_ceiling_semitones) ||
        policy.modulation_range_ceiling_semitones <= 0.0 ||
        !std::isfinite(policy.glide_minimum_net_motion_semitones) ||
        policy.glide_minimum_net_motion_semitones <= 0.0 ||
        policy.modulation_minimum_samples < 3 ||
        policy.glide_minimum_samples < 3) {
        throw std::invalid_argument("invalid pitch-motion articulation policy");
    }
}

inline pitch_motion_articulation_hypothesis analyze_in_episode_pitch_motion(
    const std::vector<pitch_motion_sample>& samples,
    double confidence,
    pitch_motion_analysis_policy policy = {}) {
    validate_pitch_motion_policy(policy);
    if (samples.size() < 2)
        throw std::invalid_argument("pitch-motion analysis requires at least two samples");
    if (confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("pitch-motion confidence must be in [0, 1]");

    const node_id episode_id = samples.front().physical_episode_id;
    if (episode_id == 0)
        throw std::invalid_argument("pitch-motion analysis requires a bounded physical episode");
    const std::string basis = samples.front().pitch_basis;
    const std::string semantics = samples.front().interval_semantics;
    if (basis.empty() || semantics.empty())
        throw std::invalid_argument("pitch-motion analysis requires explicit pitch basis and interval semantics");

    pitch_motion_articulation_hypothesis result;
    result.physical_episode_id = episode_id;
    result.confidence = confidence;
    result.source_nodes.reserve(samples.size());
    result.step_semitones.reserve(samples.size() - 1);

    double low = samples.front().log2_pitch_coordinate;
    double high = low;
    std::int8_t previous_nonzero_direction = 0;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const auto& sample = samples[index];
        if (sample.source_node == 0 || sample.physical_episode_id != episode_id)
            throw std::invalid_argument("one pitch-motion analysis cannot merge different physical episodes");
        if (sample.pitch_basis != basis || sample.interval_semantics != semantics)
            throw std::invalid_argument("pitch-motion sample changed coordinate semantics within one episode");
        if (!std::isfinite(sample.log2_pitch_coordinate))
            throw std::invalid_argument("pitch-motion coordinate must be finite");
        if (index != 0) {
            if (sample.time.domain != samples.front().time.domain ||
                sample.time.tick_rate != samples.front().time.tick_rate ||
                sample.time.loop_iteration != samples.front().time.loop_iteration ||
                sample.time.tick <= samples[index - 1].time.tick) {
                throw std::invalid_argument("pitch-motion samples require increasing compatible time coordinates");
            }
            const double motion =
                (sample.log2_pitch_coordinate - samples[index - 1].log2_pitch_coordinate) * 12.0;
            result.step_semitones.push_back(motion);
            const std::int8_t direction =
                motion > policy.stationary_tolerance_semitones ? 1 :
                (motion < -policy.stationary_tolerance_semitones ? -1 : 0);
            if (direction != 0) {
                if (previous_nonzero_direction != 0 && direction != previous_nonzero_direction)
                    ++result.direction_changes;
                previous_nonzero_direction = direction;
            }
        }
        result.source_nodes.push_back(sample.source_node);
        low = std::min(low, sample.log2_pitch_coordinate);
        high = std::max(high, sample.log2_pitch_coordinate);
    }

    result.pitch_range_semitones = (high - low) * 12.0;
    result.net_motion_semitones =
        (samples.back().log2_pitch_coordinate - samples.front().log2_pitch_coordinate) * 12.0;
    result.rearticulation_supported = false;

    const bool effectively_stationary =
        result.pitch_range_semitones <= policy.stationary_tolerance_semitones;
    const bool modulation_shape =
        samples.size() >= policy.modulation_minimum_samples &&
        result.direction_changes >= policy.modulation_minimum_direction_changes &&
        result.pitch_range_semitones <= policy.modulation_range_ceiling_semitones &&
        std::fabs(result.net_motion_semitones) <= result.pitch_range_semitones * 0.5;
    const bool monotonic_glide =
        samples.size() >= policy.glide_minimum_samples &&
        result.direction_changes == 0 &&
        std::fabs(result.net_motion_semitones) >= policy.glide_minimum_net_motion_semitones;

    if (effectively_stationary)
        result.kind = pitch_motion_articulation_kind::steady_pitch;
    else if (modulation_shape)
        result.kind = pitch_motion_articulation_kind::periodic_modulation_candidate;
    else if (monotonic_glide)
        result.kind = pitch_motion_articulation_kind::glide_candidate;
    else
        result.kind = pitch_motion_articulation_kind::in_episode_pitch_change_unresolved;

    return result;
}

inline pitch_motion_articulation_hypothesis make_rearticulation_boundary(
    node_id ending_episode,
    node_id starting_episode,
    double confidence) {
    if (ending_episode == 0 || starting_episode == 0 || ending_episode == starting_episode)
        throw std::invalid_argument("rearticulation boundary requires two distinct physical episodes");
    if (confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("rearticulation-boundary confidence must be in [0, 1]");

    pitch_motion_articulation_hypothesis result;
    result.kind = pitch_motion_articulation_kind::rearticulation_boundary;
    result.physical_episode_id = ending_episode;
    result.next_physical_episode_id = starting_episode;
    result.rearticulation_supported = true;
    result.confidence = confidence;
    return result;
}

} // namespace vgmtooling::model
