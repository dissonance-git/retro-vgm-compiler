#pragma once

#include "diatonic_key_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

inline bool compatible_collection_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration;
}

inline double equal_temperament_exact_step(
    double frequency_hz,
    const equal_temperament_model& tuning) {
    validate_equal_temperament_model(tuning);
    if (!std::isfinite(frequency_hz) || frequency_hz <= 0.0)
        throw std::invalid_argument("pitch-class projection requires finite positive frequency");
    return static_cast<double>(tuning.reference_step) +
        static_cast<double>(tuning.divisions_per_octave) *
            std::log2(frequency_hz / tuning.reference_frequency_hz);
}

inline pitch_class_collection_profile make_surface_pitch_class_collection(
    const time_span& region,
    equal_temperament_model tuning,
    const std::vector<absolute_musical_pitch_observation>& observations,
    std::string source,
    double maximum_deviation_cents = 35.0) {
    if (!region.end.has_value())
        throw std::invalid_argument("surface pitch-class collection requires a finite analysis region");
    if (!compatible_collection_time_basis(region.start, *region.end) ||
        region.end->tick <= region.start.tick) {
        throw std::invalid_argument("surface pitch-class collection requires a valid finite local time span");
    }
    if (tuning.divisions_per_octave != 12)
        throw std::invalid_argument("surface pitch-class collection currently requires explicit 12-TET projection");
    validate_equal_temperament_model(tuning);
    if (!std::isfinite(maximum_deviation_cents) || maximum_deviation_cents <= 0.0)
        throw std::invalid_argument("surface pitch-class projection tolerance must be finite and positive");
    if (source.empty())
        throw std::invalid_argument("surface pitch-class collection requires provenance source");
    if (observations.empty())
        throw std::invalid_argument("surface pitch-class collection requires pitch observations");

    pitch_class_collection_profile profile;
    profile.region = region;
    profile.tuning = std::move(tuning);
    profile.scope = pitch_class_collection_scope::surface_performance;
    profile.source = std::move(source);

    std::optional<musical_pitch_role> role;
    double total_duration = 0.0;
    double confidence_weighted_duration = 0.0;
    double projected_confidence_weighted_duration = 0.0;
    double quality_duration = 0.0;

    for (const auto& observation : observations) {
        validate_absolute_musical_pitch_observation(observation);
        if (!compatible_collection_time_basis(region.start, observation.active.start))
            throw std::invalid_argument("surface pitch-class observations require one local time basis");
        if (observation.active.end.has_value() &&
            !compatible_collection_time_basis(observation.active.start, *observation.active.end)) {
            throw std::invalid_argument("surface pitch-class observation has incompatible end time basis");
        }

        const std::int64_t overlap_start = std::max(
            region.start.tick,
            observation.active.start.tick);
        const std::int64_t observation_end = observation.active.end.has_value()
            ? observation.active.end->tick
            : region.end->tick;
        const std::int64_t overlap_end = std::min(
            region.end->tick,
            observation_end);
        if (overlap_end <= overlap_start)
            continue;

        if (!role.has_value())
            role = observation.role;
        else if (*role != observation.role)
            throw std::invalid_argument("surface pitch-class collection cannot mix programmed, performed, and heard pitch roles");

        const double duration = static_cast<double>(overlap_end - overlap_start);
        const double confidence_duration = duration * observation.confidence;
        total_duration += duration;
        confidence_weighted_duration += confidence_duration;

        const double exact_step = equal_temperament_exact_step(
            observation.frequency_hz,
            profile.tuning);
        const std::int64_t nearest_step = static_cast<std::int64_t>(std::llround(exact_step));
        const double cents =
            (exact_step - static_cast<double>(nearest_step)) * 100.0;
        const double absolute_cents = std::fabs(cents);
        if (absolute_cents > maximum_deviation_cents)
            continue;

        const double fit = std::max(
            0.0,
            1.0 - absolute_cents / maximum_deviation_cents);
        const double projected_weight = confidence_duration * fit;
        const std::int64_t pitch_class = positive_mod(nearest_step, 12);
        profile.salience[static_cast<std::size_t>(pitch_class)] += projected_weight;
        projected_confidence_weighted_duration += confidence_duration;
        quality_duration += duration * std::min(
            {observation.confidence, profile.tuning.confidence, fit});
    }

    if (!role.has_value() || total_duration <= 0.0)
        throw std::invalid_argument("no supplied pitch observation overlaps the requested collection region");
    if (confidence_weighted_duration <= 0.0)
        throw std::invalid_argument("surface pitch-class collection has no positive-confidence pitch evidence");

    double total_projected_salience = 0.0;
    for (double value : profile.salience)
        total_projected_salience += value;
    if (total_projected_salience <= 0.0)
        throw std::invalid_argument("no overlapping pitch evidence fits the supplied tuning projection");

    profile.pitch_role = *role;
    profile.projection_coverage = std::min(
        1.0,
        projected_confidence_weighted_duration / confidence_weighted_duration);
    profile.confidence = std::min(
        profile.tuning.confidence,
        quality_duration / total_duration);
    return profile;
}

} // namespace vgmtooling::model
