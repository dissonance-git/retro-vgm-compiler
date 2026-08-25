#pragma once

#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace vgmtooling::model {

enum class contextual_figuration_kind : std::uint8_t {
    passing_tone = 0,
    neighbor_tone,
};

struct contextual_tertian_figuration_hypothesis {
    time_coordinate observation_time{};
    std::int64_t root_pitch_class = 0;
    tertian_triad_quality quality = tertian_triad_quality::major;
    triad_inversion inversion = triad_inversion::unknown;
    contextual_figuration_kind figuration_kind = contextual_figuration_kind::passing_tone;
    node_id figuration_part_id = 0;
    std::int64_t previous_step = 0;
    std::int64_t figuration_step = 0;
    std::int64_t next_step = 0;
    std::int64_t figuration_pitch_class = 0;
    std::optional<std::int64_t> displaced_structural_pitch_class{};
    std::vector<std::int64_t> structural_pitch_classes;
    std::vector<std::int64_t> surface_pitch_classes;
    std::size_t retained_structural_pitch_classes = 0;
    bool surrounding_exact_triad_grounded = false;
    bool bass_remains_structural = false;
    double confidence = 0.0;
    std::string theory_scope =
        "12-TET contextual triad identity with one persistent-part passing/neighbor tone";
};

constexpr double contextual_tertian_figuration_confidence_ceiling = 0.78;
constexpr std::int64_t contextual_tertian_figuration_max_step_semitones = 2;

inline const char* to_string(contextual_figuration_kind kind) noexcept {
    switch (kind) {
    case contextual_figuration_kind::passing_tone:
        return "passing_tone";
    case contextual_figuration_kind::neighbor_tone:
        return "neighbor_tone";
    }
    return "unknown";
}

inline bool contextual_figuration_same_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline bool contextual_figuration_same_tuning_contract(
    const equal_temperament_model& first,
    const equal_temperament_model& second) noexcept {
    return first.divisions_per_octave == second.divisions_per_octave &&
        first.reference_step == second.reference_step &&
        std::fabs(first.reference_frequency_hz - second.reference_frequency_hz) <= 1.0e-9;
}

inline void validate_contextual_figuration_projection(
    const equal_temperament_pitch_projection& projection) {
    validate_equal_temperament_model(projection.tuning);
    if (projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument(
            "contextual tertian figuration currently requires explicit 12-TET projections");
    if (!std::isfinite(projection.confidence) ||
        projection.confidence < 0.0 || projection.confidence > 1.0) {
        throw std::invalid_argument(
            "contextual tertian figuration projection confidence must lie in [0, 1]");
    }
    if (projection.nearest_steps.size() != projection.source_verticality.part_ids.size())
        throw std::invalid_argument(
            "contextual tertian figuration requires one persistent-part id per projected pitch");
    if (projection.nearest_steps.size() < 3)
        throw std::invalid_argument(
            "contextual tertian figuration requires at least three projected pitches");
    for (node_id part_id : projection.source_verticality.part_ids) {
        if (part_id == 0)
            throw std::invalid_argument(
                "contextual tertian figuration requires nonzero persistent-part ids");
    }
}

inline std::set<std::int64_t> contextual_pitch_class_set(
    const equal_temperament_pitch_projection& projection) {
    std::set<std::int64_t> result;
    for (std::int64_t step : projection.nearest_steps)
        result.insert(positive_mod(step, 12));
    return result;
}

inline std::optional<std::int64_t> contextual_unique_part_step(
    const equal_temperament_pitch_projection& projection,
    node_id part_id) {
    std::optional<std::int64_t> result;
    for (std::size_t index = 0; index < projection.source_verticality.part_ids.size(); ++index) {
        if (projection.source_verticality.part_ids[index] != part_id)
            continue;
        if (result.has_value())
            throw std::invalid_argument(
                "contextual tertian figuration found simultaneous pitches for one persistent part");
        result = projection.nearest_steps[index];
    }
    return result;
}

inline std::set<std::int64_t> contextual_structural_triad_classes(
    const tertian_triad_hypothesis& triad) {
    std::set<std::int64_t> result;
    for (std::int64_t offset : triad_template(triad.quality))
        result.insert(positive_mod(triad.root_pitch_class + offset, 12));
    return result;
}

inline std::optional<contextual_figuration_kind> infer_contextual_linear_figuration_kind(
    std::int64_t previous_step,
    std::int64_t current_step,
    std::int64_t next_step) {
    const std::int64_t first_motion = current_step - previous_step;
    const std::int64_t second_motion = next_step - current_step;
    if (first_motion == 0 || second_motion == 0)
        return std::nullopt;
    if (std::llabs(first_motion) > contextual_tertian_figuration_max_step_semitones ||
        std::llabs(second_motion) > contextual_tertian_figuration_max_step_semitones) {
        return std::nullopt;
    }
    if (previous_step == next_step && first_motion == -second_motion)
        return contextual_figuration_kind::neighbor_tone;
    if ((first_motion > 0 && second_motion > 0) ||
        (first_motion < 0 && second_motion < 0)) {
        return contextual_figuration_kind::passing_tone;
    }
    return std::nullopt;
}

inline std::optional<contextual_tertian_figuration_hypothesis>
infer_contextual_tertian_figuration_hypothesis(
    const equal_temperament_pitch_projection& previous,
    const equal_temperament_pitch_projection& current,
    const equal_temperament_pitch_projection& next) {
    validate_contextual_figuration_projection(previous);
    validate_contextual_figuration_projection(current);
    validate_contextual_figuration_projection(next);

    const auto previous_time = previous.source_verticality.observation_time;
    const auto current_time = current.source_verticality.observation_time;
    const auto next_time = next.source_verticality.observation_time;
    if (!contextual_figuration_same_time_basis(previous_time, current_time) ||
        !contextual_figuration_same_time_basis(current_time, next_time) ||
        !(previous_time.tick < current_time.tick && current_time.tick < next_time.tick)) {
        throw std::invalid_argument(
            "contextual tertian figuration requires three ordered observations in one time basis");
    }
    if (previous.source_verticality.role != current.source_verticality.role ||
        current.source_verticality.role != next.source_verticality.role) {
        throw std::invalid_argument(
            "contextual tertian figuration cannot mix programmed, performed, and heard pitch roles");
    }
    if (!contextual_figuration_same_tuning_contract(previous.tuning, current.tuning) ||
        !contextual_figuration_same_tuning_contract(current.tuning, next.tuning)) {
        throw std::invalid_argument(
            "contextual tertian figuration requires one equal-temperament tuning contract");
    }

    const auto previous_candidates = infer_tertian_triad_hypotheses(previous);
    const auto next_candidates = infer_tertian_triad_hypotheses(next);
    if (previous_candidates.size() != 1 || next_candidates.size() != 1 ||
        previous_candidates.front().root_ambiguous || next_candidates.front().root_ambiguous) {
        return std::nullopt;
    }
    const auto& previous_triad = previous_candidates.front();
    const auto& next_triad = next_candidates.front();
    if (previous_triad.root_pitch_class != next_triad.root_pitch_class ||
        previous_triad.quality != next_triad.quality) {
        return std::nullopt;
    }

    // Never erase a competing exact surface chord. Contextual reduction exists
    // only for verticalities the exact triad layer cannot already explain.
    if (!infer_tertian_triad_hypotheses(current).empty())
        return std::nullopt;

    const auto structural_classes = contextual_structural_triad_classes(previous_triad);
    const auto surface_classes = contextual_pitch_class_set(current);
    std::set<std::int64_t> extra_classes;
    std::set<std::int64_t> missing_classes;
    std::set_difference(
        surface_classes.begin(), surface_classes.end(),
        structural_classes.begin(), structural_classes.end(),
        std::inserter(extra_classes, extra_classes.end()));
    std::set_difference(
        structural_classes.begin(), structural_classes.end(),
        surface_classes.begin(), surface_classes.end(),
        std::inserter(missing_classes, missing_classes.end()));
    if (extra_classes.size() != 1 || missing_classes.size() > 1)
        return std::nullopt;

    std::size_t retained = 0;
    for (std::int64_t pitch_class : structural_classes)
        retained += surface_classes.count(pitch_class) != 0 ? 1u : 0u;
    if (retained < 2)
        return std::nullopt;

    const std::int64_t extra_class = *extra_classes.begin();
    const auto lowest_step = *std::min_element(
        current.nearest_steps.begin(), current.nearest_steps.end());
    const std::int64_t bass_class = positive_mod(lowest_step, 12);
    if (bass_class == extra_class)
        return std::nullopt;

    std::set<node_id> extra_parts;
    for (std::size_t index = 0; index < current.nearest_steps.size(); ++index) {
        if (positive_mod(current.nearest_steps[index], 12) == extra_class)
            extra_parts.insert(current.source_verticality.part_ids[index]);
    }
    if (extra_parts.size() != 1)
        return std::nullopt;
    const node_id figuration_part = *extra_parts.begin();

    const auto previous_step = contextual_unique_part_step(previous, figuration_part);
    const auto current_step = contextual_unique_part_step(current, figuration_part);
    const auto next_step = contextual_unique_part_step(next, figuration_part);
    if (!previous_step.has_value() || !current_step.has_value() || !next_step.has_value())
        return std::nullopt;
    if (positive_mod(*current_step, 12) != extra_class ||
        structural_classes.count(positive_mod(*previous_step, 12)) == 0 ||
        structural_classes.count(positive_mod(*next_step, 12)) == 0) {
        return std::nullopt;
    }

    const auto motion_kind = infer_contextual_linear_figuration_kind(
        *previous_step,
        *current_step,
        *next_step);
    if (!motion_kind.has_value())
        return std::nullopt;

    contextual_tertian_figuration_hypothesis result;
    result.observation_time = current_time;
    result.root_pitch_class = previous_triad.root_pitch_class;
    result.quality = previous_triad.quality;
    result.inversion = infer_triad_inversion(
        bass_class,
        result.root_pitch_class,
        result.quality);
    result.figuration_kind = *motion_kind;
    result.figuration_part_id = figuration_part;
    result.previous_step = *previous_step;
    result.figuration_step = *current_step;
    result.next_step = *next_step;
    result.figuration_pitch_class = extra_class;
    if (missing_classes.size() == 1)
        result.displaced_structural_pitch_class = *missing_classes.begin();
    result.structural_pitch_classes.assign(
        structural_classes.begin(), structural_classes.end());
    result.surface_pitch_classes.assign(surface_classes.begin(), surface_classes.end());
    result.retained_structural_pitch_classes = retained;
    result.surrounding_exact_triad_grounded = true;
    result.bass_remains_structural = true;
    result.confidence = std::min({
        previous_triad.confidence,
        current.confidence,
        next_triad.confidence,
        contextual_tertian_figuration_confidence_ceiling,
    });
    return result;
}

} // namespace vgmtooling::model
