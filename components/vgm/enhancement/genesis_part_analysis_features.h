#pragma once

#include "genesis_analysis_features.h"
#include "../../../model/persistent_part_observation_feature.h"

#include <string>

namespace gameaudio::vgm {

inline vgmtooling::model::analysis_feature_set
extract_genesis_part_aware_performance_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id,
    const genesis_pitch_clock_context* pitch_clocks = nullptr) {
    using namespace vgmtooling::model;

    analysis_feature_set features = extract_genesis_performance_analysis_features(
        graph,
        event_id,
        pitch_clocks);

    const node* event = graph.find_node(event_id);
    const std::string source = event == nullptr || event->provenance.empty()
        ? std::string{"genesis-part-analysis"}
        : event->provenance[0].source;

    features.replace(persistent_part_identity_from_observation(
        graph,
        event_id,
        source));
    return features;
}

} // namespace gameaudio::vgm
