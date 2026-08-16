#pragma once

#include "harmonic_transition_hypothesis.h"
#include "phrase_boundary_consensus.h"
#include "voice_leading_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>

namespace vgmtooling::model {

struct cadential_arrival_hypothesis {
    time_coordinate arrival_time{};
    double phrase_boundary_confidence = 0.0;
    double harmonic_transition_confidence = 0.0;
    double voice_leading_confidence = 0.0;
    std::int64_t root_motion_semitones = 0;
    std::int64_t root_interval_class = 0;
    std::size_t common_pitch_classes = 0;
    std::int64_t total_voice_motion_semitones = 0;
    bool cross_part_phrase_grounded = false;
    bool harmonic_root_motion_reliable = false;
    bool voice_leading_grounded = false;
    bool tonal_function_named = false;
    double confidence = 0.0;
};

constexpr double phrase_harmony_arrival_ceiling = 0.69;
constexpr double inferred_voice_arrival_ceiling = 0.74;
constexpr double identity_grounded_arrival_ceiling = 0.82;
constexpr double unreliable_root_arrival_ceiling = 0.60;

inline cadential_arrival_hypothesis infer_cadential_arrival(
    const phrase_boundary_consensus& boundary,
    const harmonic_transition_hypothesis& transition,
    const std::optional<voice_leading_hypothesis>& voice_leading = std::nullopt,
    std::int64_t alignment_tolerance_ticks = 0) {
    if (alignment_tolerance_ticks < 0)
        throw std::invalid_argument("cadential-arrival alignment tolerance must be nonnegative");
    if (!compatible_phrase_boundary_time_basis(boundary.representative, transition.second_time) ||
        std::llabs(boundary.representative.tick - transition.second_time.tick) > alignment_tolerance_ticks) {
        throw std::invalid_argument("harmonic arrival does not align with the supplied phrase boundary");
    }

    cadential_arrival_hypothesis result;
    result.arrival_time = boundary.representative;
    result.phrase_boundary_confidence = boundary.confidence;
    result.harmonic_transition_confidence = transition.confidence;
    result.root_motion_semitones = transition.directed_root_motion_semitones;
    result.root_interval_class = transition.root_interval_class;
    result.common_pitch_classes = transition.common_pitch_classes;
    result.cross_part_phrase_grounded = boundary.cross_part_grounded || boundary.authored_grounded;
    result.harmonic_root_motion_reliable = transition.root_motion_reliable;

    result.confidence = std::min(
        {boundary.confidence, transition.confidence, phrase_harmony_arrival_ceiling});

    if (voice_leading.has_value()) {
        const auto& voices = *voice_leading;
        if (!compatible_phrase_boundary_time_basis(transition.first_time, voices.first_time) ||
            !compatible_phrase_boundary_time_basis(transition.second_time, voices.second_time) ||
            transition.first_time.tick != voices.first_time.tick ||
            transition.second_time.tick != voices.second_time.tick) {
            throw std::invalid_argument("cadential-arrival voice leading must describe the same harmonic transition");
        }
        result.voice_leading_confidence = voices.confidence;
        result.total_voice_motion_semitones = voices.total_absolute_motion_semitones;
        result.voice_leading_grounded = true;
        const double ceiling = voices.all_correspondence_identity_grounded
            ? identity_grounded_arrival_ceiling
            : inferred_voice_arrival_ceiling;
        result.confidence = std::min(
            {boundary.confidence, transition.confidence, voices.confidence, ceiling});
    }

    if (!result.harmonic_root_motion_reliable)
        result.confidence = std::min(result.confidence, unreliable_root_arrival_ceiling);

    // This object deliberately remains function-neutral. Even strong phrase,
    // harmony, and persistent-voice convergence does not establish tonic,
    // dominant, authentic/plagal/deceptive class, mode, or key by itself.
    result.tonal_function_named = false;
    return result;
}

} // namespace vgmtooling::model
