#pragma once

#include "part_motif_profile.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace vgmtooling::model {

enum class contrapuntal_motion_kind : std::uint8_t {
    similar = 0,
    contrary,
    oblique,
    stationary,
};

struct counterpoint_motion_profile {
    node_id first_part_id = 0;
    node_id second_part_id = 0;
    std::size_t similar_motion_count = 0;
    std::size_t contrary_motion_count = 0;
    std::size_t oblique_motion_count = 0;
    std::size_t stationary_motion_count = 0;
    std::optional<std::vector<double>> vertical_interval_octaves{};
    bool vertical_intervals_comparable = false;
    std::string interval_semantics;
    double confidence = 0.0;
};

inline std::int8_t pitch_motion_sign(double difference) noexcept {
    return difference > 1e-9 ? 1 : (difference < -1e-9 ? -1 : 0);
}

inline counterpoint_motion_profile make_counterpoint_motion_profile(
    const std::vector<part_gesture_observation>& first,
    const std::vector<part_gesture_observation>& second,
    double confidence) {
    if (first.size() < 2 || first.size() != second.size())
        throw std::invalid_argument("counterpoint profile requires equally sized synchronized part observations");
    if (confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("counterpoint confidence must be in [0, 1]");

    const node_id first_part = first.front().part_id;
    const node_id second_part = second.front().part_id;
    if (first_part == 0 || second_part == 0 || first_part == second_part)
        throw std::invalid_argument("counterpoint profile requires two distinct persistent parts");

    const std::string first_basis = first.front().pitch_basis;
    const std::string second_basis = second.front().pitch_basis;
    const std::string semantics = first.front().interval_semantics;
    if (semantics.empty() || second.front().interval_semantics != semantics)
        throw std::invalid_argument("counterpoint profile requires shared interval semantics");

    bool absolute_vertical_interval_available =
        !first_basis.empty() && first_basis == second_basis;

    counterpoint_motion_profile result;
    result.first_part_id = first_part;
    result.second_part_id = second_part;
    result.interval_semantics = semantics;
    result.confidence = confidence;

    std::vector<double> vertical_intervals;
    if (absolute_vertical_interval_available)
        vertical_intervals.reserve(first.size());

    for (std::size_t index = 0; index < first.size(); ++index) {
        const auto& first_observation = first[index];
        const auto& second_observation = second[index];
        if (first_observation.part_id != first_part || second_observation.part_id != second_part)
            throw std::invalid_argument("counterpoint observation changed persistent-part identity");
        if (first_observation.onset != second_observation.onset)
            throw std::invalid_argument("counterpoint profile currently requires synchronized observation times");
        if (!first_observation.log2_pitch_coordinate.has_value() ||
            !second_observation.log2_pitch_coordinate.has_value() ||
            !std::isfinite(*first_observation.log2_pitch_coordinate) ||
            !std::isfinite(*second_observation.log2_pitch_coordinate)) {
            throw std::invalid_argument("counterpoint profile requires pitch coordinates for both parts");
        }
        if (first_observation.interval_semantics != semantics ||
            second_observation.interval_semantics != semantics) {
            throw std::invalid_argument("counterpoint observation changed interval semantics");
        }
        if (first_observation.pitch_basis != first_basis ||
            second_observation.pitch_basis != second_basis) {
            throw std::invalid_argument("counterpoint observation changed pitch basis within a part");
        }

        if (absolute_vertical_interval_available) {
            vertical_intervals.push_back(
                *second_observation.log2_pitch_coordinate -
                *first_observation.log2_pitch_coordinate);
        }
        if (index == 0)
            continue;

        const std::int8_t first_motion = pitch_motion_sign(
            *first[index].log2_pitch_coordinate -
            *first[index - 1].log2_pitch_coordinate);
        const std::int8_t second_motion = pitch_motion_sign(
            *second[index].log2_pitch_coordinate -
            *second[index - 1].log2_pitch_coordinate);

        if (first_motion == 0 && second_motion == 0)
            ++result.stationary_motion_count;
        else if (first_motion == 0 || second_motion == 0)
            ++result.oblique_motion_count;
        else if (first_motion == second_motion)
            ++result.similar_motion_count;
        else
            ++result.contrary_motion_count;
    }

    if (absolute_vertical_interval_available) {
        result.vertical_intervals_comparable = true;
        result.vertical_interval_octaves = std::move(vertical_intervals);
    }
    return result;
}

} // namespace vgmtooling::model
