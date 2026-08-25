#pragma once

#include "part_role_window_inference.h"
#include "realtime_musical_role_hypothesis.h"
#include "spatial_source.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace vgmtooling::model {

constexpr bool realtime_role_hypothesis_uses_cue(
    const realtime_role_hypothesis& hypothesis,
    realtime_musical_role_cue cue) noexcept {
    return (hypothesis.cues & role_cue_mask(cue)) != 0;
}

constexpr std::uint32_t realtime_auditory_salience_cue_mask() noexcept {
    return role_cue_mask(realtime_musical_role_cue::relative_energy)
        | role_cue_mask(realtime_musical_role_cue::activity)
        | role_cue_mask(realtime_musical_role_cue::edge_density);
}

// Attach auditory foreground evidence to a role-window descriptor only when the
// source has already been bound to that exact persistent musical part. The
// confidence cannot exceed either the auditory hypothesis or the part-identity
// binding. Presentation-prior evidence is rejected here because feeding a role-
// derived presentation prior back into role inference would create a semantic
// feedback loop rather than an independent evidence domain.
inline bool attach_realtime_auditory_salience(
    part_role_window_descriptor& descriptor,
    const spatial_source_evidence& source,
    const realtime_musical_role_hypotheses& hypotheses) {
    validate_part_role_window_descriptor(descriptor);

    if (!source.persistent_part_present)
        return false;
    if (source.persistent_part_id == 0 ||
        !std::isfinite(source.persistent_part_confidence) ||
        source.persistent_part_confidence < 0.0f ||
        source.persistent_part_confidence > 1.0f) {
        throw std::invalid_argument("auditory role evidence requires valid persistent-part source identity");
    }
    if (source.persistent_part_id != descriptor.part_id)
        return false;

    const auto& foreground = hypotheses.foreground;
    if (!std::isfinite(foreground.score) || foreground.score < 0.0f || foreground.score > 1.0f ||
        !std::isfinite(foreground.confidence) || foreground.confidence < 0.0f ||
        foreground.confidence > 1.0f) {
        throw std::invalid_argument("auditory foreground evidence must be finite in [0, 1]");
    }
    if (!(foreground.confidence > 0.0f))
        return false;
    if (realtime_role_hypothesis_uses_cue(
            foreground,
            realtime_musical_role_cue::presentation_prior)) {
        return false;
    }
    if ((foreground.cues & realtime_auditory_salience_cue_mask()) == 0)
        return false;

    descriptor.auditory_salience = bounded_role_signal{
        static_cast<double>(foreground.score),
        std::min(
            static_cast<double>(foreground.confidence),
            static_cast<double>(source.persistent_part_confidence)),
    };
    return true;
}

} // namespace vgmtooling::model
