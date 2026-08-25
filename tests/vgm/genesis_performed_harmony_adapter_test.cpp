#include "../../components/vgm/enhancement/genesis_performed_harmony_adapter.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace vgmtooling::model;
using namespace gameaudio::vgm;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 1000, 0};
}

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{at(start), at(end)};
    return graph.add_node(std::move(episode));
}

node_id add_pitch_parameter(
    musical_execution_graph& graph,
    node_id episode_id,
    std::int64_t start,
    std::int64_t end) {
    node parameter;
    parameter.kind = node_kind::parameter;
    parameter.layer = semantic_layer::musical_performance;
    parameter.flow = flow_kind::control;
    parameter.label = "YM2612 device-native pitch control";
    parameter.active = time_span{at(start), at(end)};
    parameter.attributes.push_back({
        "parameter_kind",
        std::string{"pitch"},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "representation",
        std::string{"device_native"},
        evidence_status::derived,
        1.0,
        "",
    });
    parameter.attributes.push_back({
        "device_family",
        std::string{"YM2612"},
        evidence_status::derived,
        1.0,
        "",
    });
    const node_id parameter_id = graph.add_node(std::move(parameter));

    edge control;
    control.kind = edge_kind::controls;
    control.from = parameter_id;
    control.to = episode_id;
    graph.add_edge(std::move(control));
    return parameter_id;
}

node_id add_register_transition(
    musical_execution_graph& graph,
    std::int64_t tick,
    std::uint64_t offset,
    std::uint8_t port,
    std::uint8_t reg,
    std::uint8_t data) {
    node transition;
    transition.kind = node_kind::trace_event;
    transition.layer = semantic_layer::synthesis;
    transition.flow = flow_kind::event;
    transition.label = "YM2612 register_write";
    transition.active = time_span{at(tick), std::nullopt};
    transition.attributes.push_back({
        "device_family",
        std::string{"YM2612"},
        evidence_status::derived,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "instance",
        std::uint64_t{0},
        evidence_status::derived,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "transition_kind",
        std::string{"register_write"},
        evidence_status::derived,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "port",
        static_cast<std::uint64_t>(port),
        evidence_status::exact,
        1.0,
        "",
    });
    transition.attributes.push_back({
        "register",
        static_cast<std::uint64_t>(reg),
        evidence_status::exact,
        1.0,
        "byte",
    });
    transition.attributes.push_back({
        "data",
        static_cast<std::uint64_t>(data),
        evidence_status::exact,
        1.0,
        "byte",
    });
    transition.provenance.push_back({
        evidence_status::derived,
        1.0,
        "genesis-performed-harmony-test",
        offset,
        "synthetic exact-order register transition",
    });
    return graph.add_node(std::move(transition));
}

void add_pitch_support(
    musical_execution_graph& graph,
    node_id transition_id,
    node_id parameter_id,
    std::uint16_t fnum,
    std::uint8_t block) {
    const node* transition = graph.find_node(transition_id);
    assert(transition != nullptr && transition->active.has_value());

    edge support;
    support.kind = edge_kind::contributes_to;
    support.from = transition_id;
    support.to = parameter_id;
    support.active = transition->active;
    support.attributes.push_back({
        "device_pitch_code",
        static_cast<std::uint64_t>(fnum),
        evidence_status::derived,
        1.0,
        "",
    });
    support.attributes.push_back({
        "device_pitch_block",
        static_cast<std::uint64_t>(block),
        evidence_status::derived,
        1.0,
        "",
    });
    graph.add_edge(std::move(support));
}

void add_static_snapshot(
    musical_execution_graph& graph,
    node_id episode_id,
    std::size_t channel_index,
    std::uint16_t fnum,
    std::uint8_t block,
    std::int64_t start,
    std::uint64_t source_offset) {
    ym2612_episode_synthesis_snapshot snapshot;
    snapshot.instance = 0;
    snapshot.channel_index = channel_index;
    snapshot.channel.fnum = fnum;
    snapshot.channel.block = block;
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
        "genesis-performed-harmony-test",
        source_offset);
}

node_id add_persistent_part(
    musical_execution_graph& graph,
    const std::vector<node_id>& episodes,
    std::int64_t start,
    std::int64_t end,
    double confidence = 0.93) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = "persistent musical part";
    part.active = time_span{at(start), at(end)};
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    const node_id part_id = graph.add_node(std::move(part));

    for (const auto episode_id : episodes) {
        edge membership;
        membership.kind = edge_kind::groups_into;
        membership.from = episode_id;
        membership.to = part_id;
        graph.add_edge(std::move(membership));
    }
    return part_id;
}

bool contains_part(const harmonic_verticality& value, node_id part_id) {
    return std::find(value.part_ids.begin(), value.part_ids.end(), part_id) !=
        value.part_ids.end();
}

} // namespace

int main() {
    const genesis_pitch_clock_context clocks{
        7670454,
        0,
        "synthetic-vgm-header",
    };

    musical_execution_graph graph;

    // Part A changes pitch in its first physical episode, rearticulates at 50,
    // then loses its source-backed absolute pitch interpretation at tick 80
    // when its operator topology changes.
    const node_id a1 = add_episode(graph, 10, 50);
    const node_id a2 = add_episode(graph, 50, 100);
    const node_id a1_pitch = add_pitch_parameter(graph, a1, 10, 50);
    const node_id a2_pitch = add_pitch_parameter(graph, a2, 50, 100);

    const auto a1_initial = add_register_transition(graph, 5, 10, 0, 0xA0, 0x00);
    const auto a1_change = add_register_transition(graph, 30, 30, 0, 0xA0, 0x00);
    add_pitch_support(graph, a1_initial, a1_pitch, 1000, 4);
    add_pitch_support(graph, a1_change, a1_pitch, 1100, 4);
    add_static_snapshot(graph, a1, 0, 1000, 4, 10, 20);

    const auto a2_initial = add_register_transition(graph, 45, 40, 0, 0xA0, 0x00);
    add_pitch_support(graph, a2_initial, a2_pitch, 1200, 4);
    add_static_snapshot(graph, a2, 0, 1200, 4, 50, 50);
    add_register_transition(graph, 80, 80, 0, 0xB0, 0x06);

    const node_id part_a = add_persistent_part(graph, {a1, a2}, 10, 100);

    // Part B sustains through A's pitch change, then rearticulates at 60 while
    // A is already sounding. The verticality timeline must include both newly
    // attacked and already-sounding performed pitches.
    const node_id b1 = add_episode(graph, 10, 60);
    const node_id b2 = add_episode(graph, 60, 100);
    const node_id b1_pitch = add_pitch_parameter(graph, b1, 10, 60);
    const node_id b2_pitch = add_pitch_parameter(graph, b2, 60, 100);

    const auto b1_initial = add_register_transition(graph, 5, 11, 0, 0xA1, 0x00);
    add_pitch_support(graph, b1_initial, b1_pitch, 1500, 4);
    add_static_snapshot(graph, b1, 1, 1500, 4, 10, 21);

    const auto b2_initial = add_register_transition(graph, 55, 60, 0, 0xA1, 0x00);
    add_pitch_support(graph, b2_initial, b2_pitch, 1500, 4);
    add_static_snapshot(graph, b2, 1, 1500, 4, 60, 70);

    const node_id part_b = add_persistent_part(graph, {b1, b2}, 10, 100);

    const auto observations = collect_genesis_ym2612_performed_pitch_observations(
        graph,
        clocks,
        "genesis-performed-harmony-test");
    assert(!observations.empty());
    for (const auto& observation : observations) {
        assert(observation.role == musical_pitch_role::performed);
        assert(observation.active.end.has_value());
        assert(observation.frequency_hz > 0.0);
        assert(observation.part_id == part_a || observation.part_id == part_b);
    }

    const auto verticalities = discover_genesis_ym2612_performed_harmonic_verticalities(
        graph,
        clocks,
        "genesis-performed-harmony-test");

    // Sounding-set changes occur at A's onset/pitch change/rearticulation and
    // B's rearticulation. At tick 80 A's absolute interpretation expires, so
    // only B remains and no two-part harmonic verticality is emitted.
    assert(verticalities.size() == 4);
    assert(verticalities[0].observation_time.tick == 10);
    assert(verticalities[1].observation_time.tick == 30);
    assert(verticalities[2].observation_time.tick == 50);
    assert(verticalities[3].observation_time.tick == 60);

    for (const auto& verticality : verticalities) {
        assert(verticality.role == musical_pitch_role::performed);
        assert(verticality.part_ids.size() == 2);
        assert(contains_part(verticality, part_a));
        assert(contains_part(verticality, part_b));
        assert(verticality.confidence > 0.75);
    }

    // A's performed change alters the interval while B is held. This is a real
    // source-backed harmonic-surface change, not an onset-only chord snapshot.
    assert(verticalities[1].intervals_above_lowest_octaves !=
        verticalities[0].intervals_above_lowest_octaves);

    // Rearticulation at 60 preserves the sounding pitch relation but changes
    // source provenance, so the timeline retains a distinct evidence boundary.
    assert(verticalities[3].frequencies_hz == verticalities[2].frequencies_hz);
    assert(verticalities[3].source_nodes != verticalities[2].source_nodes);

    // Generic timeline construction fails closed when one persistent part has
    // two simultaneous pitch claims.
    auto conflicting = observations;
    auto duplicate = observations.front();
    duplicate.source_node = observations.back().source_node;
    conflicting.push_back(duplicate);
    bool overlap_rejected = false;
    try {
        (void)make_harmonic_verticality_timeline(conflicting);
    } catch (const std::invalid_argument&) {
        overlap_rejected = true;
    }
    assert(overlap_rejected);

    return 0;
}
