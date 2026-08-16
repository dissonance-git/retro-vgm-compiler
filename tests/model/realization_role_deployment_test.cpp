#include "model/realization_role_grammar_bridge.h"

#include <cmath>
#include <string>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

musical_part_role_hypothesis grounded_melodic_role(
    node_id part_id,
    double confidence = 0.86) {
    std::vector<part_role_evidence> evidence;
    evidence.push_back({
        part_role_evidence_kind::melodic_motif_prominence,
        part_role_evidence_origin::musical_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        confidence,
        "blind-motif-analysis",
        "persistent part carries structurally prominent melodic material",
        {part_id},
    });
    evidence.push_back({
        part_role_evidence_kind::auditory_salience,
        part_role_evidence_origin::auditory_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        0.84,
        "blind-auditory-analysis",
        "independent auditory analysis places the part in the foreground",
        {part_id},
    });
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        time_span{at(100), at(500)},
        confidence,
        std::move(evidence));
}

musical_part_role_hypothesis register_only_role(node_id part_id) {
    std::vector<part_role_evidence> evidence;
    evidence.push_back({
        part_role_evidence_kind::register_position,
        part_role_evidence_origin::synthesis_runtime,
        part_role_evidence_polarity::supports,
        evidence_status::derived,
        0.95,
        "blind-runtime",
        "part happens to occupy a high register",
        {part_id},
    });
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        time_span{at(100), at(500)},
        0.95,
        std::move(evidence));
}

structural_grammar_context context(
    const char* soundtrack,
    const char* work,
    const char* source) {
    return {
        soundtrack,
        work,
        composer_representation_kind::synthesis_runtime,
        source,
    };
}

} // namespace

int main() {
    {
        const auto role = grounded_melodic_role(10);
        CHECK(role.relationally_grounded);
        CHECK(role.cross_domain_grounded);
        CHECK(role.confidence >= realization_role_minimum_role_confidence);

        const auto deployment = make_realization_role_deployment_hypothesis(
            role,
            "SN76489_tone",
            realization_role_deployment_kind::independent_part,
            0.62,
            1.0);
        CHECK(deployment.persistent_part_grounded);
        CHECK(deployment.role_grounded);
        CHECK(deployment.cross_domain_role_grounded);
        CHECK(deployment.realization_grounded);
        CHECK(realization_role_deployment_creator_eligible(deployment));
        CHECK(close_enough(deployment.confidence, role.confidence));

        const auto composer_observation =
            realization_role_deployment_as_grammar_observation(
                context("sonic-3d-blast", "work-a", "blind-vgm-a"),
                deployment,
                creative_attribution_role::composer);
        CHECK(composer_observation.observation.dimension ==
            composer_grammar_dimension::arrangement_orchestration);
        CHECK(composer_observation.observation.role_scope ==
            creative_attribution_role::composer);
        CHECK(composer_observation.rule_key ==
            "realization_role:SN76489_tone;role=melodic_foreground;kind=independent_part;density=0.50-0.75");
    }

    {
        // Raw realization/register facts cannot launder themselves into a
        // melodic-PSG creator fingerprint.
        const auto weak_role = register_only_role(20);
        CHECK(close_enough(weak_role.confidence, part_role_realization_only_ceiling));
        const auto deployment = make_realization_role_deployment_hypothesis(
            weak_role,
            "SN76489_tone",
            realization_role_deployment_kind::independent_part,
            0.80,
            1.0);
        CHECK(!deployment.role_grounded);
        CHECK(!realization_role_deployment_creator_eligible(deployment));
        CHECK(deployment.confidence <= realization_role_unresolved_independence_ceiling);

        bool rejected = false;
        try {
            (void)realization_role_deployment_as_grammar_observation(
                context("sonic-3d-blast", "weak-work", "blind-runtime"),
                deployment,
                creative_attribution_role::composer);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        CHECK(rejected);
    }

    {
        const auto role_a = grounded_melodic_role(30, 0.88);
        const auto role_b = grounded_melodic_role(31, 0.82);
        const auto deployment_a = make_realization_role_deployment_hypothesis(
            role_a,
            "SN76489_tone",
            realization_role_deployment_kind::independent_part,
            0.58,
            1.0);
        const auto deployment_b = make_realization_role_deployment_hypothesis(
            role_b,
            "SN76489_tone",
            realization_role_deployment_kind::independent_part,
            0.70,
            1.0);

        const auto obs_a = realization_role_deployment_as_grammar_observation(
            context("soundtrack-a", "work-a", "blind-a"),
            deployment_a,
            creative_attribution_role::composer);
        const auto obs_b = realization_role_deployment_as_grammar_observation(
            context("soundtrack-b", "work-b", "blind-b"),
            deployment_b,
            creative_attribution_role::composer);
        CHECK(obs_a.rule_key == obs_b.rule_key);

        const auto rule = make_composer_grammar_rule(
            "candidate-composer",
            creative_attribution_role::composer,
            obs_a.rule_key,
            0.95,
            {obs_a.observation, obs_b.observation});
        CHECK(rule.cross_work_grounded);
        CHECK(rule.cross_soundtrack_grounded);
        CHECK(close_enough(rule.confidence, deployment_b.confidence));

        // The same observations do not support an arranger/programmer rule.
        const auto wrong_role = make_composer_grammar_rule(
            "candidate-arranger",
            creative_attribution_role::arranger_programmer,
            obs_a.rule_key,
            0.95,
            {obs_a.observation, obs_b.observation});
        CHECK(close_enough(
            wrong_role.confidence,
            composer_grammar_no_role_support_ceiling));
    }

    return 0;
}
