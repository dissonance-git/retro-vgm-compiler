#pragma once

#include "realtime_musical_role_hypothesis.h"

#include <algorithm>

namespace vgmtooling::model {

// Convert already-learned musical-role memory into the small presentation
// vocabulary consumed by the realtime spatial scene controller and Omniphony.
//
// This projection is deliberately one-way. It must be applied only to the
// past-only handoff prepared before rendering the current block. The projected
// presentation must never be fed back into the acoustic observer/proposer as if
// it were new source evidence, otherwise the semantic sidecar could reinforce
// its own previous guesses.
inline spatial_source_evidence project_realtime_musical_spatial_evidence(
    const spatial_source_evidence& source,
    bool roles_available,
    const realtime_musical_role_hypotheses& roles) noexcept
{
    spatial_source_evidence out = source;
    if (!roles_available)
        return out;

    const auto positive_support = [](const realtime_role_hypothesis& role) noexcept {
        return clamp_unit_interval(role.score) * clamp_unit_interval(role.confidence);
    };

    const float foundation_support = positive_support(roles.foundation);
    const float foreground_support = positive_support(roles.foreground);
    const float environment_support = positive_support(roles.environmental_layer);
    const float projected_confidence = std::max({
        foundation_support,
        foreground_support,
        environment_support,
    });

    // A classification that is confidently near zero is useful to the musical
    // model, but it is not positive permission to move a source through space.
    // Omniphony's scalar confidence therefore tracks positive presentation
    // support rather than the largest raw classifier confidence.
    if (!(projected_confidence > out.presentation.confidence))
        return out;

    // Preserve stronger source-attached presentation tendencies while allowing
    // past-only musical memory to add conservative positive support. Conflicting
    // roles can coexist here. In particular, foundation + diffuse evidence is
    // safer than erasing foundation merely to obtain a larger spatial effect.
    out.presentation.foundation = std::max(
        clamp_unit_interval(out.presentation.foundation),
        clamp_unit_interval(roles.foundation.score));
    out.presentation.foreground = std::max(
        clamp_unit_interval(out.presentation.foreground),
        clamp_unit_interval(roles.foreground.score));
    out.presentation.diffuse = std::max(
        clamp_unit_interval(out.presentation.diffuse),
        clamp_unit_interval(roles.environmental_layer.score));

    // Environmental-layer evidence can justify source extent, but it does not
    // establish a direction. Keep this deliberately below the diffuse tendency;
    // actual geometry remains Omniphony policy.
    out.presentation.width = std::max(
        clamp_unit_interval(out.presentation.width),
        0.65f * clamp_unit_interval(roles.environmental_layer.score));

    // No current role hypothesis establishes vertical register/position yet.
    // Preserve any independently supplied signed affinity unchanged.
    out.presentation.vertical_affinity = clamp_unit_gain(
        out.presentation.vertical_affinity);
    out.presentation.confidence = clamp_unit_interval(projected_confidence);
    out.presentation.authority = spatial_evidence_authority::inferred;

    // Transient-accent evidence is intentionally not mapped here. A transient
    // may be a drum hit, articulation, foreground gesture, or environmental
    // event; onset/metrical context must resolve that before it steers geometry.
    return out;
}

} // namespace vgmtooling::model
