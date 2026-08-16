#include "model/structural_composer_grammar_bridge.h"

#include <cmath>
#include <string>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
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

orchestration_transition_hypothesis melodic_deployment(double confidence) {
    orchestration_transition_hypothesis relation;
    relation.kind = orchestration_transition_kind::stable_assignment;
    relation.first_part_id = 10;
    relation.second_part_id = 10;
    relation.first_role = musical_part_role::melodic_foreground;
    relation.second_role = musical_part_role::melodic_foreground;
    relation.persistent_part_preserved = true;
    relation.role_preserved = true;
    relation.realization_comparable = false;
    relation.register_comparable = false;
    relation.musical_material_continuity_grounded = true;
    relation.musical_material_continuity_confidence = confidence;
    relation.confidence = confidence;
    return relation;
}

} // namespace

int main() {
    const auto first = melodic_deployment(0.86);
    const auto second = melodic_deployment(0.82);

    // The musical orchestration relation itself does not dictate historical
    // role. If independent evidence says a creator controls this recurring
    // deployment choice compositionally, the observation may be composer-scoped.
    const auto composer_a = orchestration_transition_as_grammar_observation(
        context("soundtrack-a", "work-a", "blind-genesis-a"),
        first,
        creative_attribution_role::composer);
    const auto composer_b = orchestration_transition_as_grammar_observation(
        context("soundtrack-b", "work-b", "blind-genesis-b"),
        second,
        creative_attribution_role::composer);

    CHECK(composer_a.observation.dimension ==
        composer_grammar_dimension::arrangement_orchestration);
    CHECK(composer_a.observation.role_scope == creative_attribution_role::composer);
    CHECK(composer_a.rule_key == composer_b.rule_key);

    const auto composer_rule = make_composer_grammar_rule(
        "candidate-composer",
        creative_attribution_role::composer,
        composer_a.rule_key,
        0.95,
        {composer_a.observation, composer_b.observation});
    CHECK(composer_rule.cross_soundtrack_grounded);
    CHECK(close_enough(composer_rule.confidence, 0.82));

    // The same structural relation can separately be evaluated as an
    // arranger/programmer habit. This is a different causal hypothesis, not a
    // reinterpretation of the composer-scoped evidence.
    const auto arranger_a = orchestration_transition_as_grammar_observation(
        context("soundtrack-a", "work-a", "blind-genesis-a"),
        first,
        creative_attribution_role::arranger_programmer);
    const auto arranger_b = orchestration_transition_as_grammar_observation(
        context("soundtrack-b", "work-b", "blind-genesis-b"),
        second,
        creative_attribution_role::arranger_programmer);

    CHECK(arranger_a.rule_key == composer_a.rule_key);
    CHECK(arranger_a.observation.role_scope ==
        creative_attribution_role::arranger_programmer);

    const auto arranger_rule = make_composer_grammar_rule(
        "candidate-arranger",
        creative_attribution_role::arranger_programmer,
        arranger_a.rule_key,
        0.95,
        {arranger_a.observation, arranger_b.observation});
    CHECK(arranger_rule.cross_soundtrack_grounded);
    CHECK(close_enough(arranger_rule.confidence, 0.82));

    // Role scope is a hard firewall. Composer-scoped orchestration observations
    // cannot accidentally support an arranger/programmer hypothesis merely
    // because the grammar dimension is named arrangement_orchestration.
    const auto leaked_arranger_rule = make_composer_grammar_rule(
        "candidate-arranger",
        creative_attribution_role::arranger_programmer,
        composer_a.rule_key,
        0.95,
        {composer_a.observation, composer_b.observation});
    CHECK(!leaked_arranger_rule.cross_work_grounded);
    CHECK(!leaked_arranger_rule.cross_soundtrack_grounded);
    CHECK(close_enough(
        leaked_arranger_rule.confidence,
        composer_grammar_no_role_support_ceiling));

    return 0;
}
