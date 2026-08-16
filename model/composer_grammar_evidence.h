#pragma once

#include "creative_attribution_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class composer_representation_kind : std::uint8_t {
    symbolic_sequence = 0,
    driver_execution,
    synthesis_runtime,
    rendered_audio,
    external_transcription,
    documentary_context,
};

enum class composer_grammar_dimension : std::uint8_t {
    melody = 0,
    bass_harmony,
    rhythm,
    phrase_form,
    motif_development,
    counterpoint_voice_leading,
    arrangement_orchestration,
    timbre_synthesis,
    performance_execution,
    soundtrack_relation,
};

enum class composer_grammar_polarity : std::uint8_t {
    supports = 0,
    counters,
};

enum class composer_confound_kind : std::uint8_t {
    patch_sample_identity = 0,
    instrumentation,
    platform,
    arranger_programmer,
    soundtrack_local_context,
    transposition,
    tempo,
    related_work_family,
};

struct composer_grammar_observation {
    std::string soundtrack_id;
    std::string work_family_id;
    composer_representation_kind representation =
        composer_representation_kind::symbolic_sequence;
    composer_grammar_dimension dimension = composer_grammar_dimension::melody;
    creative_attribution_role role_scope = creative_attribution_role::composer;
    composer_grammar_polarity polarity = composer_grammar_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
};

struct composer_grammar_intervention {
    composer_confound_kind confound = composer_confound_kind::patch_sample_identity;
    bool survived = false;
    double confidence = 0.0;
    std::string source;
    std::string detail;
};

struct composer_grammar_rule {
    std::string candidate;
    creative_attribution_role role = creative_attribution_role::composer;
    std::string rule_id;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    double independent_support_ceiling = 0.0;

    std::size_t supporting_observations = 0;
    std::size_t grounding_support_observations = 0;
    std::size_t counter_observations = 0;
    std::size_t independent_work_families = 0;
    std::size_t independent_soundtracks = 0;
    std::size_t representation_families = 0;
    std::size_t musical_dimensions = 0;

    bool cross_work_grounded = false;
    bool cross_soundtrack_grounded = false;
    bool cross_representation_grounded = false;
    bool multi_dimension_grounded = false;
    bool strong_conflict_present = false;
    bool strong_confound_failure = false;
    bool intervention_grounded = false;

    std::vector<composer_grammar_observation> observations;
    std::vector<composer_grammar_intervention> interventions;
};

// Epistemic ceilings, not calibrated probabilities.
constexpr double composer_grammar_grounding_support_threshold = 0.60;
constexpr double composer_grammar_no_role_support_ceiling = 0.35;
constexpr double composer_grammar_weak_support_ceiling = 0.49;
constexpr double composer_grammar_single_work_ceiling = 0.50;
constexpr double composer_grammar_single_soundtrack_ceiling = 0.72;
constexpr double composer_grammar_strong_conflict_ceiling = 0.49;
constexpr double composer_grammar_failed_confound_ceiling = 0.49;

inline void validate_composer_grammar_observation(
    const composer_grammar_observation& observation) {
    if (observation.soundtrack_id.empty())
        throw std::invalid_argument("composer-grammar observation requires soundtrack identity");
    if (observation.work_family_id.empty())
        throw std::invalid_argument("composer-grammar observation requires work-family identity");
    if (observation.confidence < 0.0 || observation.confidence > 1.0)
        throw std::invalid_argument("composer-grammar observation confidence must be in [0, 1]");
    if (observation.source.empty())
        throw std::invalid_argument("composer-grammar observation requires a source");
}

inline void validate_composer_grammar_intervention(
    const composer_grammar_intervention& intervention) {
    if (intervention.confidence < 0.0 || intervention.confidence > 1.0)
        throw std::invalid_argument("composer-grammar intervention confidence must be in [0, 1]");
    if (intervention.source.empty())
        throw std::invalid_argument("composer-grammar intervention requires a source");
}

inline double second_strongest_or_only_support(std::vector<double> values) {
    if (values.empty())
        return 0.0;
    std::sort(values.begin(), values.end(), std::greater<double>{});
    return values.size() >= 2 ? values[1] : values[0];
}

template <typename Key>
inline double independent_support_ceiling_from_map(const std::map<Key, double>& support) {
    std::vector<double> values;
    values.reserve(support.size());
    for (const auto& item : support)
        values.push_back(item.second);
    return second_strongest_or_only_support(std::move(values));
}

inline composer_grammar_rule make_composer_grammar_rule(
    std::string candidate,
    creative_attribution_role role,
    std::string rule_id,
    double proposed_confidence,
    std::vector<composer_grammar_observation> observations,
    std::vector<composer_grammar_intervention> interventions = {}) {
    if (candidate.empty())
        throw std::invalid_argument("composer grammar requires a candidate");
    if (rule_id.empty())
        throw std::invalid_argument("composer grammar requires a rule identity");
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("composer-grammar confidence must be in [0, 1]");
    if (observations.empty())
        throw std::invalid_argument("composer grammar requires observations");

    std::set<std::pair<std::string, std::string>> work_families;
    std::set<std::string> soundtracks;
    std::set<std::uint8_t> representations;
    std::set<std::uint8_t> dimensions;
    std::map<std::pair<std::string, std::string>, double> work_support;
    std::map<std::string, double> soundtrack_support;

    bool has_role_support = false;
    bool has_grounding_role_support = false;
    bool strong_conflict = false;
    std::size_t support_count = 0;
    std::size_t grounding_support_count = 0;
    std::size_t counter_count = 0;

    for (const auto& observation : observations) {
        validate_composer_grammar_observation(observation);

        if (observation.role_scope != role)
            continue;

        if (observation.polarity == composer_grammar_polarity::supports) {
            has_role_support = true;
            ++support_count;

            if (observation.confidence < composer_grammar_grounding_support_threshold)
                continue;

            has_grounding_role_support = true;
            ++grounding_support_count;
            const auto work_key = std::make_pair(
                observation.soundtrack_id,
                observation.work_family_id);
            work_families.insert(work_key);
            soundtracks.insert(observation.soundtrack_id);
            representations.insert(static_cast<std::uint8_t>(observation.representation));
            dimensions.insert(static_cast<std::uint8_t>(observation.dimension));
            work_support[work_key] = std::max(work_support[work_key], observation.confidence);
            soundtrack_support[observation.soundtrack_id] = std::max(
                soundtrack_support[observation.soundtrack_id],
                observation.confidence);
        } else {
            ++counter_count;
            if (observation.confidence >= 0.80)
                strong_conflict = true;
        }
    }

    bool strong_confound_failure = false;
    std::size_t strong_survived_interventions = 0;
    for (const auto& intervention : interventions) {
        validate_composer_grammar_intervention(intervention);
        if (intervention.confidence < 0.80)
            continue;
        if (intervention.survived)
            ++strong_survived_interventions;
        else
            strong_confound_failure = true;
    }

    const bool cross_work = work_families.size() >= 2;
    const bool cross_soundtrack = soundtracks.size() >= 2;
    double support_ceiling = 0.0;
    if (cross_soundtrack) {
        support_ceiling = independent_support_ceiling_from_map(soundtrack_support);
    } else if (cross_work) {
        support_ceiling = independent_support_ceiling_from_map(work_support);
    } else if (!work_support.empty()) {
        support_ceiling = independent_support_ceiling_from_map(work_support);
    }

    composer_grammar_rule result;
    result.candidate = std::move(candidate);
    result.role = role;
    result.rule_id = std::move(rule_id);
    result.proposed_confidence = proposed_confidence;
    result.independent_support_ceiling = support_ceiling;
    result.supporting_observations = support_count;
    result.grounding_support_observations = grounding_support_count;
    result.counter_observations = counter_count;
    result.independent_work_families = work_families.size();
    result.independent_soundtracks = soundtracks.size();
    result.representation_families = representations.size();
    result.musical_dimensions = dimensions.size();
    result.cross_work_grounded = cross_work;
    result.cross_soundtrack_grounded = cross_soundtrack;
    result.cross_representation_grounded = representations.size() >= 2;
    result.multi_dimension_grounded = dimensions.size() >= 2;
    result.strong_conflict_present = strong_conflict;
    result.strong_confound_failure = strong_confound_failure;
    result.intervention_grounded = strong_survived_interventions >= 2;
    result.observations = std::move(observations);
    result.interventions = std::move(interventions);

    double confidence = proposed_confidence;
    if (!has_role_support) {
        confidence = std::min(confidence, composer_grammar_no_role_support_ceiling);
    } else if (!has_grounding_role_support) {
        confidence = std::min(confidence, composer_grammar_weak_support_ceiling);
    } else {
        confidence = std::min(confidence, support_ceiling);
        if (work_families.size() < 2)
            confidence = std::min(confidence, composer_grammar_single_work_ceiling);
        else if (soundtracks.size() < 2)
            confidence = std::min(confidence, composer_grammar_single_soundtrack_ceiling);
    }

    if (strong_conflict)
        confidence = std::min(confidence, composer_grammar_strong_conflict_ceiling);
    if (strong_confound_failure)
        confidence = std::min(confidence, composer_grammar_failed_confound_ceiling);

    result.confidence = confidence;
    return result;
}

inline creative_attribution_evidence as_creator_grammar_evidence(
    const composer_grammar_rule& rule,
    std::string source = "cross-soundtrack-composer-grammar") {
    creative_attribution_evidence evidence;
    evidence.kind = creative_attribution_evidence_kind::creator_grammar;
    evidence.role_scope = rule.role;
    evidence.polarity = creative_attribution_polarity::supports;
    evidence.status = evidence_status::hypothesis;
    evidence.confidence = rule.confidence;
    evidence.source = std::move(source);
    evidence.detail =
        "rule=" + rule.rule_id +
        "; work_families=" + std::to_string(rule.independent_work_families) +
        "; soundtracks=" + std::to_string(rule.independent_soundtracks) +
        "; representations=" + std::to_string(rule.representation_families) +
        "; dimensions=" + std::to_string(rule.musical_dimensions) +
        "; grounding_support=" + std::to_string(rule.grounding_support_observations);
    return evidence;
}

} // namespace vgmtooling::model
