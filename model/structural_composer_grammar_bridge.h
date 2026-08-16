#pragma once

#include "bass_harmony_interaction.h"
#include "cadential_arrival_hypothesis.h"
#include "composer_grammar_evidence.h"
#include "counterpoint_motion_profile.h"
#include "harmonic_rhythm_profile.h"
#include "imitative_part_relation.h"
#include "orchestration_transition_hypothesis.h"
#include "section_orchestration_marker.h"
#include "section_relation_hypothesis.h"
#include "voice_leading_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

struct structural_grammar_context {
    std::string soundtrack_id;
    std::string work_family_id;
    composer_representation_kind representation =
        composer_representation_kind::synthesis_runtime;
    std::string source;
};

struct blind_structural_grammar_observation {
    std::string rule_key;
    composer_grammar_observation observation;
};

inline void validate_structural_grammar_context(
    const structural_grammar_context& context) {
    if (context.soundtrack_id.empty())
        throw std::invalid_argument("structural grammar context requires soundtrack identity");
    if (context.work_family_id.empty())
        throw std::invalid_argument("structural grammar context requires work-family identity");
    if (context.source.empty())
        throw std::invalid_argument("structural grammar context requires a source");
}

inline blind_structural_grammar_observation make_blind_structural_observation(
    const structural_grammar_context& context,
    std::string rule_key,
    composer_grammar_dimension dimension,
    creative_attribution_role role_scope,
    double confidence,
    std::string detail) {
    validate_structural_grammar_context(context);
    if (rule_key.empty())
        throw std::invalid_argument("blind structural grammar observation requires a rule key");
    if (confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("blind structural grammar confidence must be in [0, 1]");

    blind_structural_grammar_observation result;
    result.rule_key = std::move(rule_key);
    result.observation = {
        context.soundtrack_id,
        context.work_family_id,
        context.representation,
        dimension,
        role_scope,
        composer_grammar_polarity::supports,
        evidence_status::hypothesis,
        confidence,
        context.source,
        std::move(detail),
    };
    return result;
}

inline std::string quantized_nonnegative_token(double value) {
    if (!std::isfinite(value) || value < 0.0)
        throw std::invalid_argument("grammar numeric token requires a finite nonnegative value");
    const double quantized = std::round(value * 4.0) / 4.0;
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << quantized;
    return stream.str();
}

inline blind_structural_grammar_observation harmonic_rhythm_as_grammar_observation(
    const structural_grammar_context& context,
    const harmonic_rhythm_profile& profile,
    creative_attribution_role role_scope) {
    if (profile.normalized_change_gaps.empty())
        throw std::invalid_argument("harmonic-rhythm grammar observation requires a normalized change profile");

    std::string signature = "harmonic_rhythm:";
    for (std::size_t index = 0; index < profile.normalized_change_gaps.size(); ++index) {
        if (index != 0)
            signature += ",";
        signature += quantized_nonnegative_token(profile.normalized_change_gaps[index]);
    }
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::rhythm,
        role_scope,
        profile.confidence,
        "tempo-scale-invariant harmonic-change spacing; absolute source ticks remain representation-local");
}

inline blind_structural_grammar_observation voice_leading_as_grammar_observation(
    const structural_grammar_context& context,
    const voice_leading_hypothesis& voices,
    creative_attribution_role role_scope) {
    if (voices.motions.empty())
        throw std::invalid_argument("voice-leading grammar observation requires voice motions");

    const double motion_per_voice =
        static_cast<double>(voices.total_absolute_motion_semitones) /
        static_cast<double>(voices.motions.size());
    const std::string signature =
        "voice_leading:stationary=" + std::to_string(voices.stationary_voices) +
        ";up=" + std::to_string(voices.upward_voices) +
        ";down=" + std::to_string(voices.downward_voices) +
        ";motion_per_voice=" + quantized_nonnegative_token(motion_per_voice) +
        ";identity_grounded=" +
        std::string{voices.all_correspondence_identity_grounded ? "true" : "false"};

    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::counterpoint_voice_leading,
        role_scope,
        voices.confidence,
        "voice-leading relation; persistent-part correspondence outranks minimum-motion inference");
}

inline blind_structural_grammar_observation counterpoint_motion_as_grammar_observation(
    const structural_grammar_context& context,
    const counterpoint_motion_profile& profile,
    creative_attribution_role role_scope) {
    const std::string signature =
        "counterpoint_motion:similar=" + std::to_string(profile.similar_motion_count) +
        ";contrary=" + std::to_string(profile.contrary_motion_count) +
        ";oblique=" + std::to_string(profile.oblique_motion_count) +
        ";stationary=" + std::to_string(profile.stationary_motion_count) +
        ";vertical_interval=" +
        std::string{profile.vertical_intervals_comparable ? "available" : "unresolved"};
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::counterpoint_voice_leading,
        role_scope,
        profile.confidence,
        "synchronized persistent-part motion relation; absolute vertical intervals remain optional evidence");
}

inline blind_structural_grammar_observation imitation_as_grammar_observation(
    const structural_grammar_context& context,
    const imitative_part_relation_hypothesis& imitation,
    creative_attribution_role role_scope) {
    const std::string signature =
        std::string{"imitation:"} + to_string(imitation.kind) +
        ";lag=" + quantized_nonnegative_token(imitation.normalized_onset_lag);
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::counterpoint_voice_leading,
        role_scope,
        imitation.confidence,
        "motif-grounded cross-part imitation/call-response relation with tempo-normalized entry lag");
}

inline blind_structural_grammar_observation bass_harmony_as_grammar_observation(
    const structural_grammar_context& context,
    const bass_harmony_interaction_hypothesis& interaction,
    creative_attribution_role role_scope) {
    const std::string signature =
        std::string{"bass_harmony:"} + to_string(interaction.kind) +
        ";retained_upper=" + std::to_string(interaction.retained_upper_pitch_classes);
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::bass_harmony,
        role_scope,
        interaction.confidence,
        "persistent-bass relation against changing/retained upper harmony");
}

inline blind_structural_grammar_observation cadential_arrival_as_grammar_observation(
    const structural_grammar_context& context,
    const cadential_arrival_hypothesis& arrival,
    creative_attribution_role role_scope) {
    const std::string signature =
        "cadential_arrival:root_ic=" + std::to_string(arrival.root_interval_class) +
        ";common_pc=" + std::to_string(arrival.common_pitch_classes) +
        ";voices=" + std::string{arrival.voice_leading_grounded ? "true" : "false"};
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::phrase_form,
        role_scope,
        arrival.confidence,
        "function-neutral structural arrival; tonic/key/cadence class remain unresolved");
}

inline blind_structural_grammar_observation section_relation_as_grammar_observation(
    const structural_grammar_context& context,
    const section_relation_hypothesis& relation,
    creative_attribution_role role_scope) {
    const double coverage = std::min(
        relation.first_phrase_coverage,
        relation.second_phrase_coverage);
    const std::string signature =
        std::string{"section_relation:"} + to_string(relation.kind) +
        ";coverage=" + quantized_nonnegative_token(coverage);
    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::phrase_form,
        role_scope,
        relation.confidence,
        "section transformation grounded in cross-phrase relations; conventional form label unresolved");
}

inline blind_structural_grammar_observation orchestration_transition_as_grammar_observation(
    const structural_grammar_context& context,
    const orchestration_transition_hypothesis& transition,
    creative_attribution_role role_scope) {
    if (transition.kind == orchestration_transition_kind::unresolved)
        throw std::invalid_argument("unresolved orchestration transition cannot become creator-grammar evidence");

    std::string register_token = "unresolved";
    if (transition.register_comparable) {
        const double magnitude = std::fabs(transition.register_shift);
        const char* direction = transition.register_shift > 1e-9
            ? "up"
            : (transition.register_shift < -1e-9 ? "down" : "static");
        register_token = std::string{direction} + ":" + quantized_nonnegative_token(magnitude);
    }

    const std::string timbre_token = transition.realization_comparable
        ? (transition.timbre_changed ? "changed" : "stable")
        : "unresolved";

    const std::string signature =
        std::string{"orchestration:"} + to_string(transition.kind) +
        ";role=" + to_string(transition.first_role) + ">" + to_string(transition.second_role) +
        ";part_preserved=" + std::string{transition.persistent_part_preserved ? "true" : "false"} +
        ";timbre=" + timbre_token +
        ";register=" + register_token +
        ";material_grounded=" +
        std::string{transition.musical_material_continuity_grounded ? "true" : "false"};

    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::arrangement_orchestration,
        role_scope,
        transition.confidence,
        "time-local role/realization/register transition; source-native timbre identities remain representation-scoped and creator identity is not available during extraction");
}

inline blind_structural_grammar_observation section_orchestration_as_grammar_observation(
    const structural_grammar_context& context,
    const section_orchestration_marker_hypothesis& marker,
    creative_attribution_role role_scope) {
    if (marker.qualifying_transition_count == 0 || marker.confidence <= 0.0)
        throw std::invalid_argument("empty section-orchestration marker cannot become creator-grammar evidence");

    const std::string signature =
        "section_orchestration:transitions=" + std::to_string(marker.qualifying_transition_count) +
        ";parts=" + std::to_string(marker.independent_part_count) +
        ";role_changes=" + std::to_string(marker.role_change_count) +
        ";role_transfers=" + std::to_string(marker.role_transfer_count) +
        ";timbre_changes=" + std::to_string(marker.timbre_change_count) +
        ";register_changes=" + std::to_string(marker.register_change_count) +
        ";density_changes=" + std::to_string(marker.density_change_count) +
        ";multi_part=" + std::string{marker.multi_part_grounded ? "true" : "false"};

    return make_blind_structural_observation(
        context,
        signature,
        composer_grammar_dimension::arrangement_orchestration,
        role_scope,
        marker.confidence,
        "orchestration changes converge on an independently established form boundary; the orchestration reinforces form but does not create the boundary");
}

} // namespace vgmtooling::model
