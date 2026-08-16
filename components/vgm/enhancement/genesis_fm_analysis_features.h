#pragma once

#include "genesis_part_analysis_features.h"
#include "ym2612_episode_pitch_analysis.h"

#include <stdexcept>
#include <string>

namespace gameaudio::vgm {

inline vgmtooling::model::analysis_feature_set
extract_genesis_fm_part_aware_performance_analysis_features(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id event_id,
    const genesis_pitch_clock_context& pitch_clocks) {
    using namespace vgmtooling::model;

    analysis_feature_set features =
        extract_genesis_part_aware_performance_analysis_features(
            graph,
            event_id,
            &pitch_clocks);

    const node* event = graph.find_node(event_id);
    if (event == nullptr)
        throw std::invalid_argument("FM-aware Genesis analysis references an unknown event");

    const attribute* family_attribute = find_genesis_analysis_attribute(*event, "device_family");
    const auto* family = family_attribute == nullptr
        ? nullptr
        : std::get_if<std::string>(&family_attribute->value);
    if (family == nullptr || *family != "YM2612")
        return features;

    const attribute* event_kind_attribute = find_genesis_analysis_attribute(*event, "event_kind");
    const auto* event_kind = event_kind_attribute == nullptr
        ? nullptr
        : std::get_if<std::string>(&event_kind_attribute->value);
    if (event_kind == nullptr || *event_kind != "pitched_activity_onset")
        return features;

    const auto realizations = graph.edges_from(event_id, edge_kind::realizes);
    if (realizations.size() != 1 ||
        graph.find_node(realizations.front()->to) == nullptr ||
        graph.find_node(realizations.front()->to)->kind != node_kind::voice_instance) {
        features.add(unresolved_feature(
            "fm_network_periodicity_frequency_hz",
            semantic_layer::synthesis,
            feature_availability::unknown,
            "operator-aware FM analysis requires one bounded physical voice episode",
            event->provenance.empty() ? "genesis-fm-analysis" : event->provenance.front().source));
        return features;
    }

    const auto fm = extract_ym2612_episode_pitch_features(
        graph,
        realizations.front()->to,
        pitch_clocks);
    const analysis_feature* periodicity = fm.find("fm_network_periodicity_frequency_hz");
    const analysis_feature* performed = fm.find("performed_pitch_frequency_hz");
    if (periodicity == nullptr || performed == nullptr)
        throw std::logic_error("YM2612 episode pitch analysis returned an incomplete feature set");

    features.add(*periodicity);
    features.replace(*performed);
    return features;
}

} // namespace gameaudio::vgm
