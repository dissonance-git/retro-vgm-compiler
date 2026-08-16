#include "model/structural_composer_grammar_bridge.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

namespace {

orchestration_transition_hypothesis recoloring(
    node_id part_id,
    double confidence) {
    orchestration_transition_hypothesis result;
    result.kind = orchestration_transition_kind::compound_reorchestration;
    result.first_part_id = part_id;
    result.second_part_id = part_id;
    result.first_role = musical_part_role::melodic_foreground;
    result.second_role = musical_part_role::melodic_foreground;
    result.transition_time = time_coordinate{time_domain::source, 100, 44100, 0};
    result.persistent_part_preserved = true;
    result.role_preserved = true;
    result.realization_comparable = true;
    result.timbre_changed = true;
    result.register_comparable = true;
    result.register_shift = 0.5;
    result.confidence = confidence;
    return result;
}

} // namespace

int main() {
    const structural_grammar_context genesis{
        "genesis-soundtrack",
        "genesis-work-family",
        composer_representation_kind::synthesis_runtime,
        "genesis-blind-extraction",
    };
    const structural_grammar_context spc{
        "spc-soundtrack",
        "spc-work-family",
        composer_representation_kind::synthesis_runtime,
        "spc-blind-extraction",
    };

    const auto genesis_observation = orchestration_transition_as_grammar_observation(
        genesis,
        recoloring(10, 0.84),
        creative_attribution_role::composer);
    const auto spc_observation = orchestration_transition_as_grammar_observation(
        spc,
        recoloring(20, 0.81),
        creative_attribution_role::composer);

    // The grammar key describes the musical transformation, not the native
    // patch/sample namespace. Different hardware can therefore support the same
    // orchestration rule without claiming equivalent instruments.
    assert(genesis_observation.rule_key == spc_observation.rule_key);
    assert(genesis_observation.rule_key.find("compound_reorchestration") != std::string::npos);
    assert(genesis_observation.rule_key.find("melodic_foreground>melodic_foreground") != std::string::npos);
    assert(genesis_observation.rule_key.find("timbre=changed") != std::string::npos);
    assert(genesis_observation.rule_key.find("register=up:0.50") != std::string::npos);

    assert(genesis_observation.observation.soundtrack_id !=
        spc_observation.observation.soundtrack_id);
    assert(genesis_observation.observation.work_family_id !=
        spc_observation.observation.work_family_id);
    assert(genesis_observation.observation.dimension ==
        composer_grammar_dimension::arrangement_orchestration);
    assert(spc_observation.observation.dimension ==
        composer_grammar_dimension::arrangement_orchestration);

    return 0;
}
