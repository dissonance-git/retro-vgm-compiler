#pragma once

#include "orchestration_transition_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

struct section_orchestration_marker_hypothesis {
    time_coordinate boundary_time{};
    double established_boundary_confidence = 0.0;
    std::size_t qualifying_transition_count = 0;
    std::size_t independent_part_count = 0;
    std::size_t role_change_count = 0;
    std::size_t role_transfer_count = 0;
    std::size_t timbre_change_count = 0;
    std::size_t register_change_count = 0;
    std::size_t density_change_count = 0;
    bool multi_part_grounded = false;
    double weakest_transition_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double single_orchestration_marker_ceiling = 0.60;
constexpr double multi_part_orchestration_marker_ceiling = 0.86;

inline bool orchestration_transition_is_structural_change(
    const orchestration_transition_hypothesis& transition) noexcept {
    return transition.kind != orchestration_transition_kind::unresolved &&
        transition.kind != orchestration_transition_kind::stable_assignment;
}

inline section_orchestration_marker_hypothesis infer_section_orchestration_marker(
    const time_coordinate& established_boundary,
    double established_boundary_confidence,
    const std::vector<orchestration_transition_hypothesis>& transitions,
    std::int64_t alignment_tolerance_ticks = 0) {
    if (established_boundary_confidence < 0.0 || established_boundary_confidence > 1.0)
        throw std::invalid_argument("section-orchestration boundary confidence must be in [0, 1]");
    if (alignment_tolerance_ticks < 0)
        throw std::invalid_argument("section-orchestration alignment tolerance must be nonnegative");

    section_orchestration_marker_hypothesis result;
    result.boundary_time = established_boundary;
    result.established_boundary_confidence = established_boundary_confidence;
    result.weakest_transition_confidence = 1.0;

    std::set<node_id> parts;
    for (const auto& transition : transitions) {
        if (!orchestration_transition_is_structural_change(transition))
            continue;
        if (!part_role_same_time_basis(established_boundary, transition.transition_time))
            continue;
        if (std::llabs(established_boundary.tick - transition.transition_time.tick) >
            alignment_tolerance_ticks) {
            continue;
        }

        ++result.qualifying_transition_count;
        result.weakest_transition_confidence = std::min(
            result.weakest_transition_confidence,
            transition.confidence);
        if (transition.first_part_id != 0)
            parts.insert(transition.first_part_id);
        if (transition.second_part_id != 0)
            parts.insert(transition.second_part_id);

        if (!transition.role_preserved)
            ++result.role_change_count;
        if (transition.kind == orchestration_transition_kind::role_transfer)
            ++result.role_transfer_count;
        if (transition.realization_comparable && transition.timbre_changed)
            ++result.timbre_change_count;
        if (transition.register_comparable && std::fabs(transition.register_shift) > 1e-9)
            ++result.register_change_count;
        if (transition.activity_density_compared &&
            std::fabs(transition.activity_density_delta) > 1e-9) {
            ++result.density_change_count;
        }
    }

    result.independent_part_count = parts.size();
    if (result.qualifying_transition_count == 0) {
        result.weakest_transition_confidence = 0.0;
        result.confidence = 0.0;
        return result;
    }

    result.multi_part_grounded =
        result.qualifying_transition_count >= 2 && result.independent_part_count >= 2;
    const double ceiling = result.multi_part_grounded
        ? multi_part_orchestration_marker_ceiling
        : single_orchestration_marker_ceiling;
    result.confidence = std::min({
        established_boundary_confidence,
        result.weakest_transition_confidence,
        ceiling,
    });
    return result;
}

} // namespace vgmtooling::model
