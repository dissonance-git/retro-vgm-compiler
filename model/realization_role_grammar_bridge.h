#pragma once

#include "realization_role_deployment_hypothesis.h"
#include "structural_composer_grammar_bridge.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

inline std::string realization_role_density_bucket(double density) {
    if (!std::isfinite(density) || density < 0.0 || density > 1.0)
        throw std::invalid_argument("realization-role density must be in [0, 1]");
    const int bucket = std::min(3, static_cast<int>(density * 4.0));
    switch (bucket) {
    case 0:
        return "0.00-0.25";
    case 1:
        return "0.25-0.50";
    case 2:
        return "0.50-0.75";
    case 3:
        return "0.75-1.00";
    }
    return "unknown";
}

inline blind_structural_grammar_observation realization_role_deployment_as_grammar_observation(
    const structural_grammar_context& context,
    const realization_role_deployment_hypothesis& deployment,
    creative_attribution_role role_scope) {
    validate_structural_grammar_context(context);
    if (!realization_role_deployment_creator_eligible(deployment)) {
        throw std::invalid_argument(
            "realization-role deployment must be grounded in a persistent musical part and role before entering creator grammar");
    }

    blind_structural_grammar_observation result;
    result.rule_key =
        "realization_role:" + deployment.realization_family +
        ";role=" + std::string{to_string(deployment.role)} +
        ";kind=" + std::string{to_string(deployment.kind)} +
        ";density=" + realization_role_density_bucket(deployment.activity_density);

    result.observation.soundtrack_id = context.soundtrack_id;
    result.observation.work_family_id = context.work_family_id;
    result.observation.representation = context.representation;
    result.observation.dimension = composer_grammar_dimension::arrangement_orchestration;
    result.observation.role_scope = role_scope;
    result.observation.polarity = composer_grammar_polarity::supports;
    result.observation.status = evidence_status::hypothesis;
    result.observation.confidence = deployment.confidence;
    result.observation.source = context.source;
    result.observation.detail =
        "creator-blind realization-role deployment: realization=" +
        deployment.realization_family +
        "; musical_role=" + std::string{to_string(deployment.role)} +
        "; deployment_kind=" + std::string{to_string(deployment.kind)} +
        "; activity_density_bucket=" +
        realization_role_density_bucket(deployment.activity_density) +
        "; persistent-part and role grounding precede creator identity";
    return result;
}

} // namespace vgmtooling::model
