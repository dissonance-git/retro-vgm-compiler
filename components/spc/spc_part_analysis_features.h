#pragma once

#include "spc_analysis_features.h"
#include "../../model/persistent_part_observation_feature.h"

#include <string>

namespace gameaudio::spc {

inline vgmtooling::model::analysis_feature_set
extract_spc_part_aware_runtime_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id) {
    using namespace vgmtooling::model;

    analysis_feature_set features = extract_spc_runtime_analysis_features(graph, event_id);
    const node* event = graph.find_node(event_id);
    if (event == nullptr)
        return features;

    const attribute* kind_item = find_spc_analysis_attribute(*event, "event_kind");
    const auto* kind = kind_item == nullptr
        ? nullptr
        : std::get_if<std::string>(&kind_item->value);
    if (kind != nullptr && (*kind == "continuation_lost" || *kind == "execution_reset"))
        return features;

    const std::string source = event->provenance.empty()
        ? std::string{"spc-part-analysis"}
        : event->provenance[0].source;
    features.replace(persistent_part_identity_from_observation(
        graph,
        event_id,
        source));
    return features;
}

} // namespace gameaudio::spc
