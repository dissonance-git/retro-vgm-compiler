#pragma once

#include "composer_grammar_evidence.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct attribution_control_match {
    std::string query_id;
    std::string candidate;
    creative_attribution_role role = creative_attribution_role::composer;

    std::string control_id;
    std::string soundtrack_id;
    std::string work_family_id;
    std::string platform_id;
    std::string implementation_family_id;

    composer_representation_kind representation =
        composer_representation_kind::symbolic_sequence;
    composer_grammar_dimension dimension = composer_grammar_dimension::melody;
    composer_grammar_polarity polarity = composer_grammar_polarity::supports;
    evidence_status status = evidence_status::hypothesis;

    // Similarity or contradiction strength supplied by a lower musical analyzer.
    // This layer does not decide how melody, harmony, orchestration, or execution
    // similarity is measured; it decides whether that evidence survives controls.
    double match_strength = 0.0;
    double confidence = 0.0;
    std::string source;
    std::string detail;
};

struct blind_attribution_policy {
    double minimum_candidate_score = 0.65;
    double minimum_winner_margin = 0.08;
    double robustness_floor = 0.55;
    std::size_t minimum_independent_soundtracks = 2;
    std::size_t minimum_independent_work_families = 2;
    bool require_cross_implementation_support = true;
};

struct blind_attribution_request {
    std::string query_id;
    creative_attribution_role role = creative_attribution_role::composer;
    std::string query_platform_id;
    std::string query_implementation_family_id;
    blind_attribution_policy policy{};
    std::vector<attribution_control_match> matches;

    // Evidence not produced by the musical similarity pass, such as an exact
    // documentary role credit or independent version-lineage observation.
    std::map<std::string, std::vector<creative_attribution_evidence>> independent_evidence;
};

struct blind_attribution_candidate_result {
    std::string candidate;
    creative_attribution_role role = creative_attribution_role::composer;
    double score = 0.0;
    double runner_up_margin = 0.0;
    double leave_one_soundtrack_out_floor = 0.0;
    double query_platform_exclusion_score = 0.0;
    double query_implementation_exclusion_score = 0.0;

    std::size_t independent_soundtracks = 0;
    std::size_t independent_work_families = 0;
    std::size_t independent_platforms = 0;
    std::size_t independent_implementation_families = 0;
    std::size_t musical_dimensions = 0;
    std::size_t representation_families = 0;

    bool platform_exclusion_applicable = false;
    bool implementation_exclusion_applicable = false;
    bool survives_leave_one_soundtrack_out = false;
    bool survives_query_platform_exclusion = false;
    bool survives_query_implementation_exclusion = false;
    bool cross_soundtrack_grounded = false;
    bool cross_work_grounded = false;
    bool admissible = false;

    composer_grammar_rule grammar;
    creative_attribution_hypothesis attribution;
};

struct blind_attribution_result {
    std::string query_id;
    creative_attribution_role role = creative_attribution_role::composer;
    std::vector<blind_attribution_candidate_result> ranked_candidates;
    bool decisive = false;
    std::string winner;
    double winner_margin = 0.0;
};

inline void validate_attribution_control_match(const attribution_control_match& match) {
    if (match.query_id.empty())
        throw std::invalid_argument("attribution control match requires query identity");
    if (match.candidate.empty())
        throw std::invalid_argument("attribution control match requires candidate identity");
    if (match.control_id.empty())
        throw std::invalid_argument("attribution control match requires control identity");
    if (match.soundtrack_id.empty())
        throw std::invalid_argument("attribution control match requires soundtrack identity");
    if (match.work_family_id.empty())
        throw std::invalid_argument("attribution control match requires work-family identity");
    if (match.match_strength < 0.0 || match.match_strength > 1.0)
        throw std::invalid_argument("attribution match strength must be in [0, 1]");
    if (match.confidence < 0.0 || match.confidence > 1.0)
        throw std::invalid_argument("attribution match confidence must be in [0, 1]");
    if (match.source.empty())
        throw std::invalid_argument("attribution control match requires a source");
}

inline void validate_blind_attribution_policy(const blind_attribution_policy& policy) {
    const auto valid_unit = [](double value) { return value >= 0.0 && value <= 1.0; };
    if (!valid_unit(policy.minimum_candidate_score) ||
        !valid_unit(policy.minimum_winner_margin) ||
        !valid_unit(policy.robustness_floor)) {
        throw std::invalid_argument("blind-attribution policy thresholds must be in [0, 1]");
    }
    if (policy.minimum_independent_soundtracks == 0 ||
        policy.minimum_independent_work_families == 0) {
        throw std::invalid_argument("blind-attribution independence minima must be non-zero");
    }
}

namespace detail {

struct weighted_mean_accumulator {
    double weighted_sum = 0.0;
    double weight = 0.0;

    void add(double value, double confidence) {
        weighted_sum += value * confidence;
        weight += confidence;
    }

    double value() const noexcept {
        return weight > 0.0 ? weighted_sum / weight : 0.0;
    }
};

inline double effective_control_score(const attribution_control_match& match) noexcept {
    return match.polarity == composer_grammar_polarity::supports
        ? match.match_strength
        : 1.0 - match.match_strength;
}

inline double hierarchical_candidate_score(
    const std::vector<const attribution_control_match*>& matches) {
    // Equalize first by musical dimension inside each soundtrack, then by
    // soundtrack. More files or more extracted features in one soundtrack do
    // not automatically buy a candidate more weight.
    std::map<std::string, std::map<std::uint8_t, weighted_mean_accumulator>> grouped;
    for (const auto* match : matches) {
        grouped[match->soundtrack_id][static_cast<std::uint8_t>(match->dimension)].add(
            effective_control_score(*match),
            match->confidence);
    }

    if (grouped.empty())
        return 0.0;

    double soundtrack_sum = 0.0;
    std::size_t soundtrack_count = 0;
    for (const auto& soundtrack : grouped) {
        if (soundtrack.second.empty())
            continue;
        double dimension_sum = 0.0;
        for (const auto& dimension : soundtrack.second)
            dimension_sum += dimension.second.value();
        soundtrack_sum += dimension_sum / static_cast<double>(soundtrack.second.size());
        ++soundtrack_count;
    }
    return soundtrack_count > 0
        ? soundtrack_sum / static_cast<double>(soundtrack_count)
        : 0.0;
}

inline std::vector<const attribution_control_match*> without_soundtrack(
    const std::vector<const attribution_control_match*>& matches,
    const std::string& soundtrack_id) {
    std::vector<const attribution_control_match*> filtered;
    for (const auto* match : matches) {
        if (match->soundtrack_id != soundtrack_id)
            filtered.push_back(match);
    }
    return filtered;
}

inline std::vector<const attribution_control_match*> without_platform(
    const std::vector<const attribution_control_match*>& matches,
    const std::string& platform_id) {
    std::vector<const attribution_control_match*> filtered;
    for (const auto* match : matches) {
        if (match->platform_id != platform_id)
            filtered.push_back(match);
    }
    return filtered;
}

inline std::vector<const attribution_control_match*> without_implementation_family(
    const std::vector<const attribution_control_match*>& matches,
    const std::string& implementation_family_id) {
    std::vector<const attribution_control_match*> filtered;
    for (const auto* match : matches) {
        if (match->implementation_family_id != implementation_family_id)
            filtered.push_back(match);
    }
    return filtered;
}

inline double leave_one_soundtrack_out_floor(
    const std::vector<const attribution_control_match*>& matches,
    const std::set<std::string>& soundtracks) {
    if (soundtracks.size() < 2)
        return 0.0;

    double floor = 1.0;
    for (const auto& soundtrack : soundtracks) {
        const auto filtered = without_soundtrack(matches, soundtrack);
        floor = std::min(floor, hierarchical_candidate_score(filtered));
    }
    return floor;
}

inline std::vector<composer_grammar_observation> grammar_observations_from_matches(
    const std::vector<const attribution_control_match*>& matches) {
    std::vector<composer_grammar_observation> observations;
    observations.reserve(matches.size());
    for (const auto* match : matches) {
        composer_grammar_observation observation;
        observation.soundtrack_id = match->soundtrack_id;
        observation.work_family_id = match->work_family_id;
        observation.representation = match->representation;
        observation.dimension = match->dimension;
        observation.role_scope = match->role;
        observation.polarity = match->polarity;
        observation.status = match->status;
        observation.confidence = std::clamp(
            match->match_strength * match->confidence,
            0.0,
            1.0);
        observation.source = match->source;
        observation.detail = match->detail;
        observations.push_back(std::move(observation));
    }
    return observations;
}

} // namespace detail

inline blind_attribution_result run_blind_attribution_experiment(
    const blind_attribution_request& request) {
    if (request.query_id.empty())
        throw std::invalid_argument("blind attribution requires query identity");
    validate_blind_attribution_policy(request.policy);
    if (request.matches.empty())
        throw std::invalid_argument("blind attribution requires control matches");

    std::map<std::string, std::vector<const attribution_control_match*>> by_candidate;
    for (const auto& match : request.matches) {
        validate_attribution_control_match(match);
        if (match.query_id != request.query_id || match.role != request.role)
            continue;
        by_candidate[match.candidate].push_back(&match);
    }
    if (by_candidate.empty())
        throw std::invalid_argument("blind attribution has no role-matched evidence for query");

    blind_attribution_result result;
    result.query_id = request.query_id;
    result.role = request.role;

    for (const auto& candidate_entry : by_candidate) {
        const auto& candidate = candidate_entry.first;
        const auto& matches = candidate_entry.second;

        std::set<std::string> soundtracks;
        std::set<std::pair<std::string, std::string>> work_families;
        std::set<std::string> platforms;
        std::set<std::string> implementation_families;
        std::set<std::uint8_t> dimensions;
        std::set<std::uint8_t> representations;
        for (const auto* match : matches) {
            soundtracks.insert(match->soundtrack_id);
            work_families.insert({match->soundtrack_id, match->work_family_id});
            if (!match->platform_id.empty())
                platforms.insert(match->platform_id);
            if (!match->implementation_family_id.empty())
                implementation_families.insert(match->implementation_family_id);
            dimensions.insert(static_cast<std::uint8_t>(match->dimension));
            representations.insert(static_cast<std::uint8_t>(match->representation));
        }

        blind_attribution_candidate_result candidate_result;
        candidate_result.candidate = candidate;
        candidate_result.role = request.role;
        candidate_result.score = detail::hierarchical_candidate_score(matches);
        candidate_result.independent_soundtracks = soundtracks.size();
        candidate_result.independent_work_families = work_families.size();
        candidate_result.independent_platforms = platforms.size();
        candidate_result.independent_implementation_families = implementation_families.size();
        candidate_result.musical_dimensions = dimensions.size();
        candidate_result.representation_families = representations.size();
        candidate_result.cross_soundtrack_grounded =
            soundtracks.size() >= request.policy.minimum_independent_soundtracks;
        candidate_result.cross_work_grounded =
            work_families.size() >= request.policy.minimum_independent_work_families;

        candidate_result.leave_one_soundtrack_out_floor =
            detail::leave_one_soundtrack_out_floor(matches, soundtracks);
        candidate_result.survives_leave_one_soundtrack_out =
            candidate_result.leave_one_soundtrack_out_floor >= request.policy.robustness_floor;

        candidate_result.platform_exclusion_applicable = !request.query_platform_id.empty();
        if (candidate_result.platform_exclusion_applicable) {
            const auto filtered = detail::without_platform(matches, request.query_platform_id);
            candidate_result.query_platform_exclusion_score =
                detail::hierarchical_candidate_score(filtered);
            candidate_result.survives_query_platform_exclusion =
                !filtered.empty() &&
                candidate_result.query_platform_exclusion_score >= request.policy.robustness_floor;
        } else {
            candidate_result.survives_query_platform_exclusion = true;
        }

        candidate_result.implementation_exclusion_applicable =
            !request.query_implementation_family_id.empty();
        if (candidate_result.implementation_exclusion_applicable) {
            const auto filtered = detail::without_implementation_family(
                matches,
                request.query_implementation_family_id);
            candidate_result.query_implementation_exclusion_score =
                detail::hierarchical_candidate_score(filtered);
            candidate_result.survives_query_implementation_exclusion =
                !filtered.empty() &&
                candidate_result.query_implementation_exclusion_score >=
                    request.policy.robustness_floor;
        } else {
            candidate_result.survives_query_implementation_exclusion = true;
        }

        // Intervention confidence means confidence that the holdout was actually
        // performed and interpreted, not the similarity score that remained.
        // These interventions are deterministic over the supplied match set, so
        // a failed holdout must remain a high-confidence confound failure.
        std::vector<composer_grammar_intervention> interventions;
        interventions.push_back({
            composer_confound_kind::soundtrack_local_context,
            candidate_result.survives_leave_one_soundtrack_out,
            1.0,
            "blind-attribution:leave-one-soundtrack-out",
            "candidate must remain supported after each soundtrack family is removed; floor=" +
                std::to_string(candidate_result.leave_one_soundtrack_out_floor),
        });
        if (candidate_result.platform_exclusion_applicable) {
            interventions.push_back({
                composer_confound_kind::platform,
                candidate_result.survives_query_platform_exclusion,
                1.0,
                "blind-attribution:query-platform-exclusion",
                "controls sharing the query platform were removed; score=" +
                    std::to_string(candidate_result.query_platform_exclusion_score),
            });
        }
        if (candidate_result.implementation_exclusion_applicable) {
            interventions.push_back({
                composer_confound_kind::arranger_programmer,
                candidate_result.survives_query_implementation_exclusion,
                1.0,
                "blind-attribution:query-implementation-exclusion",
                "controls sharing the query implementation/driver family were removed; score=" +
                    std::to_string(candidate_result.query_implementation_exclusion_score),
            });
        }

        candidate_result.grammar = make_composer_grammar_rule(
            candidate,
            request.role,
            "blind-attribution:" + request.query_id,
            candidate_result.score,
            detail::grammar_observations_from_matches(matches),
            std::move(interventions));

        std::vector<creative_attribution_evidence> attribution_evidence;
        attribution_evidence.push_back(as_creator_grammar_evidence(
            candidate_result.grammar,
            "blind-attribution-experiment"));
        const auto independent = request.independent_evidence.find(candidate);
        if (independent != request.independent_evidence.end()) {
            attribution_evidence.insert(
                attribution_evidence.end(),
                independent->second.begin(),
                independent->second.end());
        }
        candidate_result.attribution = make_creative_attribution_hypothesis(
            candidate,
            request.role,
            candidate_result.score,
            std::move(attribution_evidence));

        result.ranked_candidates.push_back(std::move(candidate_result));
    }

    std::sort(
        result.ranked_candidates.begin(),
        result.ranked_candidates.end(),
        [](const blind_attribution_candidate_result& lhs,
           const blind_attribution_candidate_result& rhs) {
            if (lhs.score != rhs.score)
                return lhs.score > rhs.score;
            return lhs.candidate < rhs.candidate;
        });

    if (!result.ranked_candidates.empty()) {
        auto& top = result.ranked_candidates.front();
        const double runner_up_score = result.ranked_candidates.size() >= 2
            ? result.ranked_candidates[1].score
            : 0.0;
        top.runner_up_margin = top.score - runner_up_score;
        result.winner_margin = top.runner_up_margin;

        const bool implementation_ok =
            !request.policy.require_cross_implementation_support ||
            top.survives_query_implementation_exclusion;
        top.admissible =
            top.score >= request.policy.minimum_candidate_score &&
            top.runner_up_margin >= request.policy.minimum_winner_margin &&
            top.cross_soundtrack_grounded &&
            top.cross_work_grounded &&
            top.survives_leave_one_soundtrack_out &&
            top.survives_query_platform_exclusion &&
            implementation_ok;

        result.decisive = top.admissible;
        if (result.decisive)
            result.winner = top.candidate;
    }

    return result;
}

} // namespace vgmtooling::model
