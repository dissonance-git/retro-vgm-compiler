#pragma once

#include "diatonic_key_hypothesis.h"
#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

constexpr double structural_chord_collection_min_confidence = 0.70;

inline bool compatible_structural_collection_tuning(
    const equal_temperament_model& first,
    const equal_temperament_model& second) noexcept {
    return first.divisions_per_octave == second.divisions_per_octave &&
           first.reference_step == second.reference_step &&
           std::fabs(first.reference_frequency_hz - second.reference_frequency_hz) <= 1e-9;
}

inline pitch_class_collection_profile make_structural_pitch_class_collection_from_triads(
    const time_span& region,
    const std::vector<tertian_triad_hypothesis>& triads,
    std::string source,
    double minimum_chord_confidence = structural_chord_collection_min_confidence) {
    if (!region.end.has_value() || region.end->tick <= region.start.tick)
        throw std::invalid_argument("structural chord collection requires a finite nonempty region");
    if (triads.empty())
        throw std::invalid_argument("structural chord collection requires at least one triad hypothesis");
    if (source.empty())
        throw std::invalid_argument("structural chord collection requires provenance source");
    if (!std::isfinite(minimum_chord_confidence) ||
        minimum_chord_confidence < 0.0 || minimum_chord_confidence > 1.0) {
        throw std::invalid_argument("minimum chord confidence must lie in [0, 1]");
    }

    const auto& first_projection = triads.front().projection;
    validate_equal_temperament_model(first_projection.tuning);
    if (first_projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("structural diatonic pitch collection currently requires explicit 12-TET chord projections");

    pitch_class_collection_profile profile;
    profile.region = region;
    profile.tuning = first_projection.tuning;
    profile.pitch_role = first_projection.source_verticality.role;
    profile.scope = pitch_class_collection_scope::structural_hypothesis;
    profile.projection_coverage = 1.0;
    profile.confidence = 1.0;
    profile.source = std::move(source);

    for (const auto& triad : triads) {
        if (!std::isfinite(triad.confidence) || triad.confidence < 0.0 || triad.confidence > 1.0)
            throw std::invalid_argument("triad confidence must lie in [0, 1]");
        if (triad.confidence < minimum_chord_confidence)
            throw std::invalid_argument("weak triad cannot be silently included in structural pitch-class collection");
        if (triad.pitch_classes.size() != 3)
            throw std::invalid_argument("structural triad collection requires exactly three pitch classes per hypothesis");
        if (!compatible_structural_collection_tuning(profile.tuning, triad.projection.tuning))
            throw std::invalid_argument("structural chord collection cannot mix tuning contracts");
        if (triad.projection.source_verticality.role != profile.pitch_role)
            throw std::invalid_argument("structural chord collection cannot mix programmed, performed, and heard pitch roles");
        if (!time_coordinate_inside_span(
                triad.projection.source_verticality.observation_time,
                region)) {
            throw std::invalid_argument("triad observation lies outside structural pitch-class region");
        }

        profile.confidence = std::min(
            {profile.confidence,
             triad.confidence,
             triad.projection.confidence,
             triad.projection.tuning.confidence});
        for (std::int64_t pitch_class : triad.pitch_classes) {
            profile.salience[static_cast<std::size_t>(positive_mod(pitch_class, 12))] +=
                triad.confidence;
        }
    }

    if (profile.confidence < minimum_chord_confidence)
        throw std::invalid_argument("structural chord collection confidence fell below required threshold");
    validate_pitch_class_collection_profile(profile);
    return profile;
}

} // namespace vgmtooling::model
