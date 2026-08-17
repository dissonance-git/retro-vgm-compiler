#pragma once

#include "blind_attribution_experiment.h"
#include "part_motif_profile.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
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

// Exact maximum-weight one-to-one assignment for a rectangular matrix. The
// Hungarian solve removes profile-enumeration order from cue-level evidence.
// Rows or columns may outnumber the other side; every item on the smaller side
// is matched because motif identity weights are nonnegative, while unmatched
// items on the larger side remain zero through the cue-level denominator.
inline std::vector<std::pair<std::size_t, std::size_t>>
maximum_weight_part_motif_assignment(const std::vector<std::vector<double>>& weights) {
    if (weights.empty() || weights.front().empty())
        return {};
    const std::size_t row_count = weights.size();
    const std::size_t column_count = weights.front().size();
    for (const auto& row : weights) {
        if (row.size() != column_count)
            throw std::invalid_argument("part-motif assignment matrix must be rectangular");
        for (double weight : row) {
            if (!std::isfinite(weight) || weight < 0.0)
                throw std::invalid_argument("part-motif assignment weights must be finite and nonnegative");
        }
    }

    const bool transposed = row_count > column_count;
    std::vector<std::vector<double>> matrix;
    if (transposed) {
        matrix.assign(column_count, std::vector<double>(row_count, 0.0));
        for (std::size_t row = 0; row < row_count; ++row) {
            for (std::size_t column = 0; column < column_count; ++column)
                matrix[column][row] = weights[row][column];
        }
    } else {
        matrix = weights;
    }

    const std::size_t n = matrix.size();
    const std::size_t m = matrix.front().size();
    std::vector<double> u(n + 1, 0.0);
    std::vector<double> v(m + 1, 0.0);
    std::vector<std::size_t> p(m + 1, 0);
    std::vector<std::size_t> way(m + 1, 0);

    for (std::size_t i = 1; i <= n; ++i) {
        p[0] = i;
        std::size_t j0 = 0;
        std::vector<double> minv(m + 1, std::numeric_limits<double>::infinity());
        std::vector<bool> used(m + 1, false);
        do {
            used[j0] = true;
            const std::size_t i0 = p[j0];
            double delta = std::numeric_limits<double>::infinity();
            std::size_t j1 = 0;
            for (std::size_t j = 1; j <= m; ++j) {
                if (used[j])
                    continue;
                const double current = -matrix[i0 - 1][j - 1] - u[i0] - v[j];
                if (current < minv[j]) {
                    minv[j] = current;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }
            for (std::size_t j = 0; j <= m; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            const std::size_t j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    std::vector<std::pair<std::size_t, std::size_t>> result;
    result.reserve(n);
    for (std::size_t column = 1; column <= m; ++column) {
        if (p[column] == 0)
            continue;
        const std::size_t row = p[column] - 1;
        const std::size_t col = column - 1;
        result.push_back(transposed
            ? std::pair<std::size_t, std::size_t>{col, row}
            : std::pair<std::size_t, std::size_t>{row, col});
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Compare two cue-level sets of persistent-part profiles without any creator or
// catalog labels. Candidate pairs are assigned one-to-one by globally maximum
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

    std::vector<std::vector<part_motif_similarity>> similarities(
        query.size(),
        std::vector<part_motif_similarity>(control.size()));
    std::vector<std::vector<double>> weights(
        query.size(),
        std::vector<double>(control.size(), 0.0));
    for (std::size_t qi = 0; qi < query.size(); ++qi) {
        for (std::size_t ci = 0; ci < control.size(); ++ci) {
            similarities[qi][ci] = compare_part_motif_profiles(query[qi], control[ci]);
            weights[qi][ci] = similarities[qi][ci].identity_confidence;
        }
    }

    const auto assignment = maximum_weight_part_motif_assignment(weights);
    double score_sum = 0.0;
    for (const auto& [qi, ci] : assignment) {
        const auto& similarity = similarities[qi][ci];
        score_sum += similarity.identity_confidence;
        ++result.matched_pair_count;
        if (similarity.pitch_comparable)
            ++result.pitch_comparable_pair_count;
        result.matches.push_back({qi, ci, similarity});
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
