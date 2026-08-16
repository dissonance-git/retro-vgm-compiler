#pragma once

#include "voice_leading_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

enum class bass_harmony_interaction_kind : std::uint8_t {
    unresolved = 0,
    pedal_bass_under_harmonic_change,
    moving_bass_under_retained_upper_material,
    inversion_or_bass_revoicing,
    generic_harmonic_change,
    harmonic_identity_retained,
};

struct bass_harmony_interaction_hypothesis {
    bass_harmony_interaction_kind kind = bass_harmony_interaction_kind::unresolved;
    node_id bass_part_id = 0;
    std::int64_t bass_motion_semitones = 0;
    std::size_t retained_upper_pitch_classes = 0;
    bool harmonic_identity_changed = false;
    bool bass_identity_grounded = false;
    double confidence = 0.0;
};

constexpr double unresolved_bass_identity_ceiling = 0.55;

inline const char* to_string(bass_harmony_interaction_kind kind) noexcept {
    switch (kind) {
    case bass_harmony_interaction_kind::unresolved:
        return "unresolved";
    case bass_harmony_interaction_kind::pedal_bass_under_harmonic_change:
        return "pedal_bass_under_harmonic_change";
    case bass_harmony_interaction_kind::moving_bass_under_retained_upper_material:
        return "moving_bass_under_retained_upper_material";
    case bass_harmony_interaction_kind::inversion_or_bass_revoicing:
        return "inversion_or_bass_revoicing";
    case bass_harmony_interaction_kind::generic_harmonic_change:
        return "generic_harmonic_change";
    case bass_harmony_interaction_kind::harmonic_identity_retained:
        return "harmonic_identity_retained";
    }
    return "unknown";
}

inline std::set<std::int64_t> upper_pitch_classes(
    const tertian_triad_hypothesis& chord) {
    if (chord.projection.nearest_steps.empty())
        throw std::invalid_argument("bass-harmony interaction requires projected chord pitches");
    std::set<std::int64_t> classes;
    for (std::size_t index = 1; index < chord.projection.nearest_steps.size(); ++index)
        classes.insert(positive_mod(chord.projection.nearest_steps[index], 12));
    return classes;
}

inline bass_harmony_interaction_hypothesis infer_bass_harmony_interaction(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    const voice_leading_hypothesis& voices) {
    validate_voice_leading_projection(first);
    validate_voice_leading_projection(second);
    if (first.projection.nearest_steps.size() != second.projection.nearest_steps.size())
        throw std::invalid_argument("bass-harmony interaction requires equal verticality cardinality");
    if (first.projection.nearest_steps.empty())
        throw std::invalid_argument("bass-harmony interaction requires at least one voice");

    bass_harmony_interaction_hypothesis result;
    result.confidence = std::min({first.confidence, second.confidence, voices.confidence});
    result.harmonic_identity_changed =
        first.root_pitch_class != second.root_pitch_class || first.quality != second.quality;

    const node_id first_bass_part = first.projection.source_verticality.part_ids.front();
    const node_id second_bass_part = second.projection.source_verticality.part_ids.front();
    if (first_bass_part == 0 || first_bass_part != second_bass_part) {
        result.kind = bass_harmony_interaction_kind::unresolved;
        result.confidence = std::min(result.confidence, unresolved_bass_identity_ceiling);
        return result;
    }

    const auto motion = std::find_if(
        voices.motions.begin(),
        voices.motions.end(),
        [&](const voice_leading_motion& item) {
            return item.first_part_id == first_bass_part &&
                item.second_part_id == second_bass_part &&
                item.persistent_identity_preserved;
        });
    if (motion == voices.motions.end()) {
        result.kind = bass_harmony_interaction_kind::unresolved;
        result.confidence = std::min(result.confidence, unresolved_bass_identity_ceiling);
        return result;
    }

    result.bass_identity_grounded = true;
    result.bass_part_id = first_bass_part;
    result.bass_motion_semitones = motion->semitone_motion;

    const auto first_upper = upper_pitch_classes(first);
    const auto second_upper = upper_pitch_classes(second);
    for (std::int64_t pitch_class : first_upper)
        result.retained_upper_pitch_classes += second_upper.count(pitch_class) != 0 ? 1u : 0u;

    if (!result.harmonic_identity_changed) {
        result.kind = result.bass_motion_semitones == 0
            ? bass_harmony_interaction_kind::harmonic_identity_retained
            : bass_harmony_interaction_kind::inversion_or_bass_revoicing;
    } else if (result.bass_motion_semitones == 0) {
        result.kind = bass_harmony_interaction_kind::pedal_bass_under_harmonic_change;
    } else if (result.retained_upper_pitch_classes != 0) {
        result.kind = bass_harmony_interaction_kind::moving_bass_under_retained_upper_material;
    } else {
        result.kind = bass_harmony_interaction_kind::generic_harmonic_change;
    }
    return result;
}

} // namespace vgmtooling::model
