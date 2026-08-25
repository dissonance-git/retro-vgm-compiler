#pragma once

#include "musical_part_role_hypothesis.h"
#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace vgmtooling::model {

// Cadence morphology needs a melodic arrival, not merely the highest sounding
// pitch in a verticality. In reconstructed game-music textures the foreground
// line may sit below a doubling, countermelody, or ornamental source, so this
// evidence binds one projected arrival pitch to a persistent part whose
// time-local melodic-foreground role was established independently.
struct cadential_melodic_arrival_evidence {
    time_coordinate arrival_time{};
    node_id part_id = 0;
    std::int64_t projected_step = 0;
    std::int64_t pitch_class = 0;
    musical_pitch_role pitch_role = musical_pitch_role::programmed;
    bool melodic_role_grounded = false;
    bool role_explicit_grounded = false;
    bool role_relationally_grounded = false;
    bool role_cross_domain_grounded = false;
    double confidence = 0.0;
};

constexpr double cadential_melodic_arrival_confidence_ceiling = 0.82;
constexpr double cadential_melodic_role_use_threshold = 0.55;

inline bool cadential_melodic_time_inside_role(
    const time_coordinate& coordinate,
    const time_span& active) noexcept {
    if (!active.end.has_value() ||
        !part_role_same_time_basis(coordinate, active.start) ||
        !part_role_same_time_basis(coordinate, *active.end)) {
        return false;
    }
    return coordinate.tick >= active.start.tick &&
        coordinate.tick < active.end->tick;
}

inline std::optional<cadential_melodic_arrival_evidence>
infer_cadential_melodic_arrival_evidence(
    const tertian_triad_hypothesis& arrival_chord,
    const musical_part_role_hypothesis& melodic_role) {
    if (!std::isfinite(arrival_chord.confidence) ||
        arrival_chord.confidence < 0.0 || arrival_chord.confidence > 1.0 ||
        !std::isfinite(arrival_chord.projection.confidence) ||
        arrival_chord.projection.confidence < 0.0 ||
        arrival_chord.projection.confidence > 1.0) {
        throw std::invalid_argument(
            "cadential melodic arrival chord confidence must be finite in [0, 1]");
    }
    if (!std::isfinite(melodic_role.confidence) ||
        melodic_role.confidence < 0.0 || melodic_role.confidence > 1.0) {
        throw std::invalid_argument(
            "cadential melodic role confidence must be finite in [0, 1]");
    }
    if (melodic_role.part_id == 0)
        throw std::invalid_argument(
            "cadential melodic role requires a nonzero persistent-part id");

    // A non-melodic or weak/realization-only role is valid input, but it does
    // not license cadence-melody evidence. Missing evidence remains distinct
    // from malformed evidence.
    if (melodic_role.role != musical_part_role::melodic_foreground ||
        melodic_role.realization_only ||
        (!melodic_role.explicit_role_grounded && !melodic_role.relationally_grounded) ||
        melodic_role.confidence < cadential_melodic_role_use_threshold) {
        return std::nullopt;
    }

    const auto arrival_time = arrival_chord.projection.source_verticality.observation_time;
    if (!cadential_melodic_time_inside_role(arrival_time, melodic_role.active))
        return std::nullopt;

    const auto& part_ids = arrival_chord.projection.source_verticality.part_ids;
    const auto& steps = arrival_chord.projection.nearest_steps;
    if (part_ids.size() != steps.size())
        throw std::invalid_argument(
            "cadential melodic arrival requires one persistent-part id per projected pitch");
    if (steps.empty())
        return std::nullopt;

    std::optional<std::size_t> matched_index;
    for (std::size_t index = 0; index < part_ids.size(); ++index) {
        if (part_ids[index] != melodic_role.part_id)
            continue;
        if (matched_index.has_value())
            throw std::invalid_argument(
                "cadential melodic arrival is ambiguous because one persistent part has multiple simultaneous pitches");
        matched_index = index;
    }
    if (!matched_index.has_value())
        return std::nullopt;

    cadential_melodic_arrival_evidence result;
    result.arrival_time = arrival_time;
    result.part_id = melodic_role.part_id;
    result.projected_step = steps[*matched_index];
    result.pitch_class = positive_mod(result.projected_step, 12);
    result.pitch_role = arrival_chord.projection.source_verticality.role;
    result.melodic_role_grounded = true;
    result.role_explicit_grounded = melodic_role.explicit_role_grounded;
    result.role_relationally_grounded = melodic_role.relationally_grounded;
    result.role_cross_domain_grounded = melodic_role.cross_domain_grounded;
    result.confidence = std::min({
        arrival_chord.confidence,
        arrival_chord.projection.confidence,
        melodic_role.confidence,
        cadential_melodic_arrival_confidence_ceiling,
    });
    return result;
}

} // namespace vgmtooling::model
