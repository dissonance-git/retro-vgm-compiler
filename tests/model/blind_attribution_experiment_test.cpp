#include "model/blind_attribution_experiment.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

namespace {

attribution_control_match support(
    std::string query,
    std::string candidate,
    creative_attribution_role role,
    std::string control,
    std::string soundtrack,
    std::string work,
    std::string platform,
    std::string implementation,
    composer_grammar_dimension dimension,
    double strength,
    double confidence = 0.95) {
    attribution_control_match match;
    match.query_id = std::move(query);
    match.candidate = std::move(candidate);
    match.role = role;
    match.control_id = std::move(control);
    match.soundtrack_id = std::move(soundtrack);
    match.work_family_id = std::move(work);
    match.platform_id = std::move(platform);
    match.implementation_family_id = std::move(implementation);
    match.representation = composer_representation_kind::driver_execution;
    match.dimension = dimension;
    match.polarity = composer_grammar_polarity::supports;
    match.status = evidence_status::derived;
    match.match_strength = strength;
    match.confidence = confidence;
    match.source = "frozen-blind-control";
    match.detail = "synthetic regression standing in for a lower musical analyzer";
    return match;
}

} // namespace

int main() {
    blind_attribution_request request;
    request.query_id = "unknown-sonic3-cue";
    request.role = creative_attribution_role::composer;
    request.query_platform_id = "mega-drive";
    request.query_implementation_family_id = "smps-sonic3";

    // Candidate A resembles controls across unrelated soundtracks and survives
    // removal of the query platform and driver family.
    request.matches.push_back(support(
        request.query_id, "candidate-A", creative_attribution_role::composer,
        "A-ancient-magic", "ancient-magic", "battle-family", "snes", "nspc",
        composer_grammar_dimension::melody, 0.88));
    request.matches.push_back(support(
        request.query_id, "candidate-A", creative_attribution_role::composer,
        "A-independent-game", "independent-game", "stage-family", "snes", "nspc",
        composer_grammar_dimension::rhythm, 0.84));
    request.matches.push_back(support(
        request.query_id, "candidate-A", creative_attribution_role::composer,
        "A-sonic-known", "sonic3-known-control", "boss-family", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::phrase_form, 0.96));

    // Candidate B has one tempting same-driver match, but unrelated controls
    // are materially weaker.
    request.matches.push_back(support(
        request.query_id, "candidate-B", creative_attribution_role::composer,
        "B-other-1", "other-game-1", "stage-family", "snes", "nspc",
        composer_grammar_dimension::melody, 0.54));
    request.matches.push_back(support(
        request.query_id, "candidate-B", creative_attribution_role::composer,
        "B-other-2", "other-game-2", "boss-family", "arcade", "custom-driver",
        composer_grammar_dimension::rhythm, 0.52));
    request.matches.push_back(support(
        request.query_id, "candidate-B", creative_attribution_role::composer,
        "B-sonic-known", "sonic3-known-control", "zone-family", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::phrase_form, 0.80));

    // A realization/programming fingerprint must not leak into composer
    // ranking even when it is numerically perfect.
    request.matches.push_back(support(
        request.query_id, "implementer-only", creative_attribution_role::arranger_programmer,
        "impl-1", "sonic3-impl-a", "impl-family-a", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::performance_execution, 0.99));
    request.matches.push_back(support(
        request.query_id, "implementer-only", creative_attribution_role::arranger_programmer,
        "impl-2", "sonic3-impl-b", "impl-family-b", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::arrangement_orchestration, 0.99));

    // Real-world-shaped role decoy: surviving Battle Master album credits assign
    // Masanori Hikichi arrangement on J.S.P.-composed material. Even a perfect
    // structural match therefore remains arranger/programmer evidence and is
    // invisible to this composer-scoped request.
    request.matches.push_back(support(
        request.query_id, "Masanori Hikichi", creative_attribution_role::arranger_programmer,
        "battle-master-arrangement-decoy", "battle-master", "jsp-composition",
        "snes", "cube-battle-master",
        composer_grammar_dimension::arrangement_orchestration, 1.0, 1.0));

    const auto result = run_blind_attribution_experiment(request);
    assert(result.ranked_candidates.size() == 2);
    assert(result.ranked_candidates.front().candidate == "candidate-A");
    assert(result.decisive);
    assert(result.winner == "candidate-A");
    assert(result.winner_margin >= request.policy.minimum_winner_margin);

    const auto& winner = result.ranked_candidates.front();
    assert(winner.cross_soundtrack_grounded);
    assert(winner.cross_work_grounded);
    assert(winner.survives_leave_one_soundtrack_out);
    assert(winner.survives_query_platform_exclusion);
    assert(winner.survives_query_implementation_exclusion);
    assert(winner.query_implementation_exclusion_score > 0.80);
    assert(winner.admissible);

    // The experiment can choose a blind candidate, while the attribution layer
    // still records creator grammar as hypothesis evidence rather than direct
    // documentary proof.
    assert(winner.attribution.candidate == "candidate-A");
    assert(winner.attribution.role == creative_attribution_role::composer);
    assert(!winner.attribution.evidence.empty());
    assert(winner.attribution.evidence.front().kind ==
           creative_attribution_evidence_kind::creator_grammar);

    // A candidate supported only by the query implementation can score highly
    // in-sample yet must fail the implementation-family intervention.
    blind_attribution_request confounded;
    confounded.query_id = "confounded-query";
    confounded.role = creative_attribution_role::composer;
    confounded.query_platform_id = "mega-drive";
    confounded.query_implementation_family_id = "smps-sonic3";
    confounded.matches.push_back(support(
        confounded.query_id, "same-driver-only", creative_attribution_role::composer,
        "c1", "sonic3-control-a", "family-a", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::melody, 0.98));
    confounded.matches.push_back(support(
        confounded.query_id, "same-driver-only", creative_attribution_role::composer,
        "c2", "sonic3-control-b", "family-b", "mega-drive", "smps-sonic3",
        composer_grammar_dimension::rhythm, 0.97));
    confounded.matches.push_back(support(
        confounded.query_id, "decoy", creative_attribution_role::composer,
        "d1", "outside-a", "family-a", "snes", "nspc",
        composer_grammar_dimension::melody, 0.50));
    confounded.matches.push_back(support(
        confounded.query_id, "decoy", creative_attribution_role::composer,
        "d2", "outside-b", "family-b", "arcade", "other-driver",
        composer_grammar_dimension::rhythm, 0.49));

    const auto confounded_result = run_blind_attribution_experiment(confounded);
    const auto& confounded_winner = confounded_result.ranked_candidates.front();
    assert(confounded_winner.candidate == "same-driver-only");
    assert(confounded_winner.score > 0.90);
    assert(!confounded_winner.survives_query_implementation_exclusion);
    assert(confounded_winner.grammar.strong_confound_failure);
    assert(confounded_winner.grammar.confidence <= composer_grammar_failed_confound_ceiling);
    assert(!confounded_winner.admissible);
    assert(!confounded_result.decisive);

    return 0;
}
