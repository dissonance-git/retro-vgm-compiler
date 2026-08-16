#pragma once

#include "blind_attribution_experiment.h"
#include "part_motif_profile.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct part_motif_pair_match {
    std::size_t query_index = 0;
    std::size_t control_index = 0;
    part_motif_similarity similarity{};
};

struct part_motif_set_similarity {
    std::size_t query_profile_count = 0;
    std::size_t control_profile_count = 0;
    std::size_t matched_pair_count = 0;
    std::size_t pitch_comparable_pair_count = 0;
    double matched_coverage = 0.0;
    double similarity = 0.0;
    std::vector<part_motif_pair_match> matches;
};

// Compare two cue-level sets of persistent-part profiles without any creator or
// catalog labels. Candidate pairs are greedily assigned one-to-one in descending
// evidence-bounded motif identity. The final denominator is max(|query|,|control|),
// so unmatched parts contribute zero rather than disappearing from the score.
inline part_motif_set_similarity compare_part_motif_profile_sets(
    const std::vector<part_motif_profile>& query,
    const std::vector<part_motif_profile>& control) {
    part_motif_set_similarity result;
    result.query_profile_count = query.size();
    result.control_profile_count = control.size();
    if (query.empty() || control.empty())
        return result;

    struct candidate_pair {
        std::size_t query_index = 0;
        std::size_t control_index = 0;
        part_motif_similarity similarity{};
    };

    std::vector<candidate_pair> candidates;
    candidates.reserve(query.size() * control.size());
    for (std::size_t qi = 0; qi < query.size(); ++qi) {
        for (std::size_t ci = 0; ci < control.size(); ++ci) {
            candidates.push_back({
                qi,
                ci,
                compare_part_motif_profiles(query[qi], control[ci]),
            });
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.similarity.identity_confidence != rhs.similarity.identity_confidence)
            return lhs.similarity.identity_confidence > rhs.similarity.identity_confidence;
        if (lhs.similarity.combined_similarity != rhs.similarity.combined_similarity)
            return lhs.similarity.combined_similarity > rhs.similarity.combined_similarity;
        if (lhs.query_index != rhs.query_index)
            return lhs.query_index < rhs.query_index;
        return lhs.control_index < rhs.control_index;
    });

    std::vector<bool> query_used(query.size(), false);
    std::vector<bool> control_used(control.size(), false);
    double score_sum = 0.0;
    for (const auto& candidate : candidates) {
        if (query_used[candidate.query_index] || control_used[candidate.control_index])
            continue;
        query_used[candidate.query_index] = true;
        control_used[candidate.control_index] = true;
        score_sum += candidate.similarity.identity_confidence;
        ++result.matched_pair_count;
        if (candidate.similarity.pitch_comparable)
            ++result.pitch_comparable_pair_count;
        result.matches.push_back({
            candidate.query_index,
            candidate.control_index,
            candidate.similarity,
        });
    }

    const std::size_t denominator = std::max(query.size(), control.size());
    result.matched_coverage = denominator == 0
        ? 0.0
        : static_cast<double>(result.matched_pair_count) /
              static_cast<double>(denominator);
    result.similarity = denominator == 0
        ? 0.0
        : score_sum / static_cast<double>(denominator);
    return result;
}

// Label join happens here, after both feature sets already exist. This bridge is
// intentionally composer-scoped: synthesis/runtime realization evidence is not
// reinterpreted as arrangement or programming authorship by this helper.
inline attribution_control_match make_part_motif_composer_control_match(
    std::string query_id,
    std::string candidate,
    std::string control_id,
    std::string soundtrack_id,
    std::string work_family_id,
    std::string platform_id,
    std::string implementation_family_id,
    double control_admission_confidence,
    const part_motif_set_similarity& similarity,
    std::string control_admission_source) {
    if (control_admission_confidence < 0.0 || control_admission_confidence > 1.0)
        throw std::invalid_argument("part-motif control admission confidence must be in [0, 1]");
    if (control_admission_source.empty())
        throw std::invalid_argument("part-motif control match requires admission provenance");

    attribution_control_match match;
    match.query_id = std::move(query_id);
    match.candidate = std::move(candidate);
    match.role = creative_attribution_role::composer;
    match.control_id = std::move(control_id);
    match.soundtrack_id = std::move(soundtrack_id);
    match.work_family_id = std::move(work_family_id);
    match.platform_id = std::move(platform_id);
    match.implementation_family_id = std::move(implementation_family_id);
    match.representation = composer_representation_kind::synthesis_runtime;
    match.dimension = composer_grammar_dimension::melody;
    match.polarity = composer_grammar_polarity::supports;
    match.status = evidence_status::hypothesis;
    match.match_strength = similarity.similarity;
    match.confidence = control_admission_confidence;
    match.source = "blind-part-motif:synthesis-runtime";
    match.detail =
        "label joined after chip-neutral motif extraction; matched_parts=" +
        std::to_string(similarity.matched_pair_count) +
        "/" + std::to_string(std::max(
            similarity.query_profile_count,
            similarity.control_profile_count)) +
        "; pitch_comparable_pairs=" +
        std::to_string(similarity.pitch_comparable_pair_count) +
        "; control_admission_source=" + control_admission_source;
    validate_attribution_control_match(match);
    return match;
}

} // namespace vgmtooling::model
