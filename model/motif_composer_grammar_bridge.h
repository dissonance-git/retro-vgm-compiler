#pragma once

#include "composer_grammar_evidence.h"
#include "part_motif_profile.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

struct motif_grammar_context {
    std::string soundtrack_id;
    std::string work_family_id;
    composer_representation_kind representation = composer_representation_kind::synthesis_runtime;
    std::string source;
};

struct motif_grammar_observation_pair {
    composer_grammar_observation first;
    composer_grammar_observation second;
};

inline void validate_motif_grammar_context(const motif_grammar_context& context) {
    if (context.soundtrack_id.empty())
        throw std::invalid_argument("motif grammar context requires soundtrack identity");
    if (context.work_family_id.empty())
        throw std::invalid_argument("motif grammar context requires work-family identity");
    if (context.source.empty())
        throw std::invalid_argument("motif grammar context requires a source");
}

inline composer_grammar_observation motif_similarity_as_grammar_observation(
    const motif_grammar_context& context,
    const part_motif_similarity& similarity,
    creative_attribution_role role_scope,
    std::string detail = {}) {
    validate_motif_grammar_context(context);

    if (detail.empty()) {
        detail = "motif relation identity_confidence=" +
            std::to_string(similarity.identity_confidence) +
            "; pitch_comparable=" +
            std::string{similarity.pitch_comparable ? "true" : "false"} +
            "; transposition_invariant=" +
            std::string{similarity.transposition_invariant ? "true" : "false"} +
            "; tempo_scale_invariant=" +
            std::string{similarity.tempo_scale_invariant ? "true" : "false"};
    }

    return {
        context.soundtrack_id,
        context.work_family_id,
        context.representation,
        composer_grammar_dimension::motif_development,
        role_scope,
        composer_grammar_polarity::supports,
        evidence_status::hypothesis,
        similarity.identity_confidence,
        context.source,
        std::move(detail),
    };
}

inline motif_grammar_observation_pair motif_similarity_as_grammar_observations(
    const motif_grammar_context& first,
    const motif_grammar_context& second,
    const part_motif_similarity& similarity,
    creative_attribution_role role_scope,
    std::string relation_detail = {}) {
    validate_motif_grammar_context(first);
    validate_motif_grammar_context(second);

    const std::string shared_detail = relation_detail.empty()
        ? "cross-work motif correspondence"
        : std::move(relation_detail);

    return {
        motif_similarity_as_grammar_observation(
            first,
            similarity,
            role_scope,
            shared_detail + "; paired_with=" + second.soundtrack_id + "/" + second.work_family_id),
        motif_similarity_as_grammar_observation(
            second,
            similarity,
            role_scope,
            shared_detail + "; paired_with=" + first.soundtrack_id + "/" + first.work_family_id),
    };
}

} // namespace vgmtooling::model
