#include "../../components/vgm/enhancement/genesis_persistent_performance_adapter.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;
using namespace gameaudio::vgm;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

struct episode_fixture {
    node_id episode_id = 0;
    node_id pitch_parameter_id = 0;
};

episode_fixture add_ym2612_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    const std::vector<std::uint16_t>& fnums,
    std::uint64_t program_fingerprint = 0x1234u,
    bool add_snapshot = true) {
    assert(!fnums.empty());

    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{at(start), at(end)};
    episode.attributes.push_back({
        "device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "instrument_program_fingerprint",
        program_fingerprint,
        evidence_status::derived,
        1.0,
        "fnv1a64",
    });
    const node_id episode_id = graph.add_node(std::move(episode));

    node onset;
    onset.kind = node_kind::musical_event;
    onset.layer = semantic_layer::musical_performance;
    onset.flow = flow_kind::event;
    onset.label = "YM2612 pitched activity onset";
    onset.active = time_span{at(start), std::nullopt};
    onset.attributes.push_back({
        "event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_pitch_code",
        static_cast<std::uint64_t>(fnums.front()),
        evidence_status::derived,
        1.0,
        "device_native",
    });
    onset.attributes.push_back({
        "device_pitch_block", std::uint64_t{4}, evidence_status::derived, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realization;
    realization.kind = edge_kind::realizes;
    realization.from = onset_id;
    realization.to = episode_id;
    graph.add_edge(std::move(realization));

    node parameter;
    parameter.kind = node_kind::parameter;
    parameter.layer = semantic_layer::musical_performance;
    parameter.flow = flow_kind::control;
    parameter.label = "YM2612 device-native pitch control";
    parameter.active = time_span{at(start), at(end)};
    parameter.attributes.push_back({
        "parameter_kind", std::string{"pitch"}, evidence_status::derived, 1.0, ""});
    parameter.attributes.push_back({
        "representation", std::string{"device_native"}, evidence_status::derived, 1.0, ""});
    parameter.attributes.push_back({
        "device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    const node_id parameter_id = graph.add_node(std::move(parameter));

    edge control;
    control.kind = edge_kind::controls;
    control.from = parameter_id;
    control.to = episode_id;
    graph.add_edge(std::move(control));

    for (std::size_t index = 0; index < fnums.size(); ++index) {
        const std::int64_t tick = start + static_cast<std::int64_t>(index * 10);
        node source;
        source.kind = node_kind::trace_event;
        source.layer = semantic_layer::synthesis;
        source.flow = flow_kind::event;
        source.label = "synthetic exact pitch source";
        source.active = time_span{at(tick), std::nullopt};
        const node_id source_id = graph.add_node(std::move(source));

        edge support;
        support.kind = edge_kind::contributes_to;
        support.from = source_id;
        support.to = parameter_id;
        support.active = time_span{at(tick), std::nullopt};
        support.attributes.push_back({
            "device_pitch_code",
            static_cast<std::uint64_t>(fnums[index]),
            evidence_status::derived,
            1.0,
            "device_native",
        });
        support.attributes.push_back({
            "device_pitch_block", std::uint64_t{4}, evidence_status::derived, 1.0, "device_native"});
        graph.add_edge(std::move(support));
    }

    if (add_snapshot) {
        ym2612_episode_synthesis_snapshot snapshot;
        snapshot.instance = 0;
        snapshot.channel_index = 0;
        snapshot.channel.fnum = fnums.front();
        snapshot.channel.block = 4;
        snapshot.channel.algorithm = 7;
        snapshot.channel.feedback = 0;
        snapshot.channel.operator_key_mask = 0x0F;
        snapshot.channel.key_on = true;
        snapshot.channel.ams = 0;
        snapshot.channel.fms = 0;
        for (auto& op : snapshot.channel.operators) {
            op.multiple = 1;
            op.detune = 0;
            op.total_level = 0;
        }
        snapshot.lfo_enabled = false;
        snapshot.lfo_frequency = 0;
        add_ym2612_episode_synthesis_snapshot(
            graph,
            episode_id,
            snapshot,
            at(start),
            "persistent-performance-test");
    }

    return {episode_id, parameter_id};
}

} // namespace

int main() {
    const genesis_pitch_clock_context clocks{
        7670454,
        3579545,
        "synthetic-vgm-header",
    };
    const genesis_part_continuity_policy continuity{
        10,
        2.0,
    };

    {
        musical_execution_graph graph;
        const auto first = add_ym2612_episode(graph, 10, 40, {1000});
        const auto second = add_ym2612_episode(graph, 45, 95, {1000, 1050, 1100, 1150});
        const auto third = add_ym2612_episode(graph, 100, 170, {1000, 1050, 950, 1050, 1000});

        const auto performance = infer_genesis_ym2612_persistent_performance(
            graph,
            {first.episode_id, second.episode_id, third.episode_id},
            clocks,
            "persistent-performance-test",
            continuity);
        assert(performance.has_value());
        assert(performance->identity.subject_nodes.size() == 3);
        assert(performance->segments.size() == 3);
        assert(performance->rearticulation_boundaries.size() == 2);

        // One trustworthy pitch state is retained without pretending that its
        // unobserved in-episode motion was stationary.
        assert(performance->segments[0].samples.size() == 1);
        assert(performance->segments[0].articulation.kind ==
               pitch_motion_articulation_kind::in_episode_pitch_change_unresolved);
        assert(performance->segments[1].articulation.kind ==
               pitch_motion_articulation_kind::glide_candidate);
        assert(performance->segments[2].articulation.kind ==
               pitch_motion_articulation_kind::periodic_modulation_candidate);
        assert(performance->rearticulation_boundaries[0].physical_episode_id == first.episode_id);
        assert(performance->rearticulation_boundaries[0].next_physical_episode_id == second.episode_id);
        assert(performance->rearticulation_boundaries[1].physical_episode_id == second.episode_id);
        assert(performance->rearticulation_boundaries[1].next_physical_episode_id == third.episode_id);
        assert(performance->confidence >= persistent_part_trajectory_link_threshold);
    }

    {
        musical_execution_graph graph;
        const auto first = add_ym2612_episode(graph, 10, 40, {1000});
        const auto second = add_ym2612_episode(
            graph,
            45,
            95,
            {1000, 1050},
            0x1234u,
            false);

        // Persistent identity may be strong, but performed reconstruction must
        // fail closed when one episode lacks a source-backed operator snapshot.
        const auto performance = infer_genesis_ym2612_persistent_performance(
            graph,
            {first.episode_id, second.episode_id},
            clocks,
            "missing-snapshot-test",
            continuity);
        assert(!performance.has_value());
    }

    {
        musical_execution_graph graph;
        const auto first = add_ym2612_episode(graph, 10, 40, {1000}, 0x1111u);
        const auto second = add_ym2612_episode(graph, 200, 250, {1000}, 0x2222u);

        // Same hardware slot plus similar pitch is not enough to manufacture a
        // persistent musical part when program identity and temporal adjacency
        // both disappear.
        const auto performance = infer_genesis_ym2612_persistent_performance(
            graph,
            {first.episode_id, second.episode_id},
            clocks,
            "weak-identity-test",
            continuity);
        assert(!performance.has_value());
    }

    return 0;
}
