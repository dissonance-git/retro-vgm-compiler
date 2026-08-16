#include "../../components/vgm/enhancement/ym2612_performed_pitch_motion.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

using namespace vgmtooling::model;
using namespace gameaudio::vgm;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

node_id add_episode(musical_execution_graph& graph) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{at(10), at(100)};
    return graph.add_node(std::move(episode));
}

node_id add_pitch_parameter(musical_execution_graph& graph, node_id episode_id) {
    node parameter;
    parameter.kind = node_kind::parameter;
    parameter.layer = semantic_layer::musical_performance;
    parameter.flow = flow_kind::control;
    parameter.label = "YM2612 device-native pitch control";
    parameter.active = time_span{at(10), at(100)};
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
        "ym2612-motion-test",
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

void add_static_snapshot(musical_execution_graph& graph, node_id episode_id) {
    ym2612_episode_synthesis_snapshot snapshot;
    snapshot.instance = 0;
    snapshot.channel_index = 0;
    snapshot.channel.fnum = 1000;
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
        at(10),
        "ym2612-motion-test",
        std::uint64_t{20});
}

struct fixture {
    musical_execution_graph graph;
    node_id episode_id = 0;
    node_id parameter_id = 0;
};

fixture make_glide_fixture() {
    fixture result;
    result.episode_id = add_episode(result.graph);
    result.parameter_id = add_pitch_parameter(result.graph, result.episode_id);

    // The first FNUM is established before key-on. Its source provenance stays
    // at tick 5, but performed motion must begin at the sounding onset, tick 10.
    const auto initial = add_register_transition(result.graph, 5, 10, 0, 0xA0, 0x00);
    add_pitch_support(result.graph, initial, result.parameter_id, 1000, 4);

    const auto step1 = add_register_transition(result.graph, 20, 30, 0, 0xA0, 0x00);
    const auto step2 = add_register_transition(result.graph, 30, 40, 0, 0xA0, 0x00);
    const auto step3 = add_register_transition(result.graph, 40, 50, 0, 0xA0, 0x00);
    add_pitch_support(result.graph, step1, result.parameter_id, 1100, 4);
    add_pitch_support(result.graph, step2, result.parameter_id, 1200, 4);
    add_pitch_support(result.graph, step3, result.parameter_id, 1300, 4);

    add_static_snapshot(result.graph, result.episode_id);
    return result;
}

} // namespace

int main() {
    const genesis_pitch_clock_context clocks{
        7670454,
        0,
        "synthetic-vgm-header",
    };

    {
        auto fixture = make_glide_fixture();
        const auto projection = project_ym2612_performed_pitch_motion(
            fixture.graph,
            fixture.parameter_id,
            clocks);
        assert(projection.static_operator_network_grounded);
        assert(projection.nominal_sample_count == 4);
        assert(projection.samples.size() == 4);
        assert(projection.pre_episode_states_rebased == 1);
        assert(projection.samples.front().time.tick == 10);
        assert(!projection.invalidating_transition_id.has_value());
        assert(projection.confidence > 0.80);

        const auto motion = analyze_ym2612_performed_pitch_motion(
            fixture.graph,
            fixture.parameter_id,
            clocks);
        assert(motion.has_value());
        assert(motion->kind == pitch_motion_articulation_kind::glide_candidate);
        assert(motion->physical_episode_id == fixture.episode_id);
        assert(motion->net_motion_semitones > 4.0);
        assert(motion->direction_changes == 0);
    }

    {
        auto fixture = make_glide_fixture();

        // Same chip, different channel. It must not terminate channel 0's
        // operator-network interpretation.
        add_register_transition(fixture.graph, 25, 35, 0, 0xB1, 0x07);
        const auto projection = project_ym2612_performed_pitch_motion(
            fixture.graph,
            fixture.parameter_id,
            clocks);
        assert(!projection.invalidating_transition_id.has_value());
        assert(projection.samples.size() == 4);
    }

    {
        auto fixture = make_glide_fixture();

        // A target-channel algorithm write changes carrier topology. The
        // performed-pitch trajectory must stop before that transition rather
        // than treating later FNUM motion as if the onset patch were unchanged.
        const auto invalidator = add_register_transition(
            fixture.graph,
            25,
            35,
            0,
            0xB0,
            0x06);
        const auto projection = project_ym2612_performed_pitch_motion(
            fixture.graph,
            fixture.parameter_id,
            clocks);
        assert(projection.invalidating_transition_id == invalidator);
        assert(projection.invalidating_time.has_value());
        assert(projection.invalidating_time->tick == 25);
        assert(projection.samples.size() == 2);
        assert(projection.samples.back().time.tick == 20);
    }

    {
        auto fixture = make_glide_fixture();

        // A post-onset key-mask write is an articulation/periodicity boundary.
        // The physical episode may remain open at the lower layer, but a static
        // all-operators-keyed pitch hypothesis may not flow through it.
        const auto invalidator = add_register_transition(
            fixture.graph,
            25,
            35,
            0,
            0x28,
            0x70);
        const auto projection = project_ym2612_performed_pitch_motion(
            fixture.graph,
            fixture.parameter_id,
            clocks);
        assert(projection.invalidating_transition_id == invalidator);
        assert(projection.samples.size() == 2);
    }

    return 0;
}
