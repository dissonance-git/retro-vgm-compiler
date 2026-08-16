#include "model/structural_composer_grammar_bridge.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

int main() {
    section_orchestration_marker_hypothesis marker;
    marker.boundary_time = time_coordinate{time_domain::source, 100, 44100, 0};
    marker.established_boundary_confidence = 0.90;
    marker.qualifying_transition_count = 3;
    marker.independent_part_count = 3;
    marker.role_change_count = 1;
    marker.role_transfer_count = 0;
    marker.timbre_change_count = 2;
    marker.register_change_count = 2;
    marker.density_change_count = 1;
    marker.multi_part_grounded = true;
    marker.weakest_transition_confidence = 0.82;
    marker.confidence = 0.82;

    const structural_grammar_context context{
        "held-out-soundtrack",
        "work-family-a",
        composer_representation_kind::synthesis_runtime,
        "blind-orchestration-analysis",
    };

    const auto observation = section_orchestration_as_grammar_observation(
        context,
        marker,
        creative_attribution_role::composer);

    assert(observation.observation.dimension ==
        composer_grammar_dimension::arrangement_orchestration);
    assert(observation.observation.confidence == 0.82);
    assert(observation.rule_key.find("section_orchestration") == 0);
    assert(observation.rule_key.find("transitions=3") != std::string::npos);
    assert(observation.rule_key.find("parts=3") != std::string::npos);
    assert(observation.rule_key.find("timbre_changes=2") != std::string::npos);
    assert(observation.rule_key.find("multi_part=true") != std::string::npos);

    return 0;
}
