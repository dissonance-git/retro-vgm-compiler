#pragma once

#include "bass_harmony_interaction.h"
#include "counterpoint_motion_profile.h"
#include "imitative_part_relation.h"
#include "musical_part_role_hypothesis.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

inline part_role_evidence bass_foundation_evidence_from_harmony(
    const bass_harmony_interaction_hypothesis& interaction,
    node_id part_id,
    std::string source) {
    if (source.empty())
        throw std::invalid_argument("bass-role bridge requires a source");
    if (!interaction.bass_identity_grounded || interaction.bass_part_id == 0 ||
        interaction.bass_part_id != part_id) {
        throw std::invalid_argument("bass-role bridge requires grounded ownership by the requested persistent part");
    }

    return {
        part_role_evidence_kind::harmonic_bass_ownership,
        part_role_evidence_origin::musical_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        interaction.confidence,
        std::move(source),
        std::string{"persistent part is the grounded lowest harmonic voice across a "} +
            to_string(interaction.kind) + " relation",
        {part_id},
    };
}

inline part_role_evidence counterline_evidence_from_counterpoint(
    const counterpoint_motion_profile& profile,
    node_id part_id,
    std::string source) {
    if (source.empty())
        throw std::invalid_argument("counterline-role bridge requires a source");
    if (part_id == 0 ||
        (part_id != profile.first_part_id && part_id != profile.second_part_id)) {
        throw std::invalid_argument("counterline-role bridge requires one of the profiled persistent parts");
    }

    const std::size_t moving_relations =
        profile.similar_motion_count + profile.contrary_motion_count + profile.oblique_motion_count;
    if (moving_relations == 0)
        throw std::invalid_argument("counterline-role bridge requires actual inter-part motion evidence");

    // Counterpoint proves independent relational behavior, not foreground rank.
    // Keep this useful but bounded so another dimension must establish a strong
    // counterline role rather than equating any second voice with counterline.
    return {
        part_role_evidence_kind::counterpoint_independence,
        part_role_evidence_origin::musical_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        std::min(profile.confidence, 0.72),
        std::move(source),
        "persistent part participates in synchronized independent motion; foreground/counterline hierarchy remains unresolved by counterpoint alone",
        {profile.first_part_id, profile.second_part_id},
    };
}

inline part_role_evidence response_evidence_from_imitation(
    const imitative_part_relation_hypothesis& imitation,
    node_id responding_part_id,
    std::string source) {
    if (source.empty())
        throw std::invalid_argument("imitation-role bridge requires a source");
    if (responding_part_id == 0 || responding_part_id != imitation.second_part_id)
        throw std::invalid_argument("imitation-role bridge requires the responding persistent part");
    if (imitation.kind == imitative_part_relation_kind::weak_relation)
        throw std::invalid_argument("weak imitation relation cannot support a part-role observation");

    return {
        part_role_evidence_kind::imitation_or_response,
        part_role_evidence_origin::musical_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        imitation.confidence,
        std::move(source),
        std::string{"responding part enters after another part with "} +
            to_string(imitation.kind) + " material; response relation does not by itself decide foreground rank",
        {imitation.first_part_id, imitation.second_part_id},
    };
}

inline part_role_evidence realization_context_evidence(
    part_role_evidence_kind kind,
    double confidence,
    std::string source,
    std::string detail,
    std::vector<node_id> support_nodes = {}) {
    if (!realization_only_part_role_evidence(kind))
        throw std::invalid_argument("realization-context bridge accepts only register/density/timbre evidence");
    if (confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("realization-context confidence must be in [0, 1]");
    if (source.empty())
        throw std::invalid_argument("realization-context evidence requires a source");

    return {
        kind,
        part_role_evidence_origin::synthesis_runtime,
        part_role_evidence_polarity::supports,
        evidence_status::derived,
        confidence,
        std::move(source),
        std::move(detail),
        std::move(support_nodes),
    };
}

} // namespace vgmtooling::model
