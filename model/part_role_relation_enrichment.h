#pragma once

#include "bass_harmony_interaction.h"
#include "counterpoint_motion_profile.h"
#include "imitative_part_relation.h"
#include "part_role_window_inference.h"
#include "phrase_boundary_hypothesis.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

inline void merge_role_signal_max(
    std::optional<bounded_role_signal>& target,
    bounded_role_signal signal) {
    validate_bounded_role_signal(signal);
    if (!target.has_value() ||
        bounded_role_signal_strength(signal) > bounded_role_signal_strength(*target)) {
        target = signal;
    }
}

inline void enrich_role_descriptor_with_bass_harmony(
    part_role_window_descriptor& descriptor,
    const bass_harmony_interaction_hypothesis& interaction) {
    if (!interaction.bass_identity_grounded || interaction.bass_part_id == 0 ||
        interaction.bass_part_id != descriptor.part_id) {
        return;
    }
    merge_role_signal_max(
        descriptor.harmonic_bass_ownership,
        bounded_role_signal{1.0, interaction.confidence});
}

inline void enrich_role_descriptor_with_counterpoint(
    part_role_window_descriptor& descriptor,
    const counterpoint_motion_profile& profile) {
    if (descriptor.part_id != profile.first_part_id && descriptor.part_id != profile.second_part_id)
        return;
    const std::size_t moving =
        profile.similar_motion_count + profile.contrary_motion_count + profile.oblique_motion_count;
    if (moving == 0)
        return;

    // Counterpoint establishes independent motion but not hierarchy. Preserve
    // the same 0.72 ceiling used by the direct role-evidence bridge.
    merge_role_signal_max(
        descriptor.counterpoint_independence,
        bounded_role_signal{1.0, std::min(profile.confidence, 0.72)});
}

inline void enrich_role_descriptor_with_imitation(
    part_role_window_descriptor& descriptor,
    const imitative_part_relation_hypothesis& imitation) {
    if (descriptor.part_id != imitation.second_part_id ||
        imitation.kind == imitative_part_relation_kind::weak_relation) {
        return;
    }
    merge_role_signal_max(
        descriptor.imitation_or_response,
        bounded_role_signal{1.0, imitation.confidence});
}

inline void enrich_role_descriptor_with_phrase_boundaries(
    part_role_window_descriptor& descriptor,
    const std::vector<phrase_boundary_hypothesis>& boundaries) {
    double strongest = 0.0;
    for (const auto& boundary : boundaries) {
        if (!part_role_same_time_basis(descriptor.active.start, boundary.boundary))
            continue;
        if (!descriptor.active.end.has_value() ||
            boundary.boundary.tick < descriptor.active.start.tick ||
            boundary.boundary.tick > descriptor.active.end->tick) {
            continue;
        }
        strongest = std::max(strongest, boundary.confidence);
    }
    if (strongest > 0.0) {
        merge_role_signal_max(
            descriptor.phrase_boundary_participation,
            bounded_role_signal{1.0, strongest});
    }
}

inline void enrich_role_descriptor_with_auditory_salience(
    part_role_window_descriptor& descriptor,
    double salience,
    double confidence) {
    merge_role_signal_max(
        descriptor.auditory_salience,
        bounded_role_signal{salience, confidence});
}

inline void enrich_role_descriptor_with_structural_motif_prominence(
    part_role_window_descriptor& descriptor,
    double prominence,
    double confidence) {
    // This helper deliberately requires a caller/analysis that has already
    // established *prominence*. Local recurrence alone is not promoted here.
    merge_role_signal_max(
        descriptor.structural_motif_prominence,
        bounded_role_signal{prominence, confidence});
}

inline void enrich_role_descriptor_with_percussion_identity(
    part_role_window_descriptor& descriptor,
    double identity_strength,
    double confidence) {
    merge_role_signal_max(
        descriptor.percussion_identity,
        bounded_role_signal{identity_strength, confidence});
}

inline void enrich_role_descriptor_with_sustained_texture(
    part_role_window_descriptor& descriptor,
    double support_strength,
    double confidence) {
    merge_role_signal_max(
        descriptor.sustained_texture,
        bounded_role_signal{support_strength, confidence});
}

inline void enrich_role_descriptor_with_doubling(
    part_role_window_descriptor& descriptor,
    double correspondence_strength,
    double confidence) {
    merge_role_signal_max(
        descriptor.doubling_correspondence,
        bounded_role_signal{correspondence_strength, confidence});
}

} // namespace vgmtooling::model
