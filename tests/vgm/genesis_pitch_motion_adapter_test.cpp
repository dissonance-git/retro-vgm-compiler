#include "components/vgm/enhancement/genesis_pitch_motion_adapter.h"

#include <cmath>
#include <cstdint>
#include <string>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

node_id add_transition(musical_execution_graph& graph, std::int64_t tick) {
    node transition;
    transition.kind = node_kind::trace_event;
    transition.layer = semantic_layer::synthesis;
    transition.flow = flow_kind::event;
    transition.active = time_span{at(tick), std::nullopt};
    return graph.add_node(std::move(transition));
}

edge_id add_pitch_support(
    musical_execution_graph& graph,
    node_id transition,
    node_id parameter,
    std::int64_t tick,
    std::uint64_t fnum,
    std::uint64_t block) {
    edge support;
    support.kind = edge_kind::contributes_to;
    support.from = transition;
    support.to = parameter;
    support.active = time_span{at(tick), std::nullopt};
    support.attributes.push_back({
        "device_pitch_code",
        fnum,
        evidence_status::derived,
        1.0,
        "device_native",
    });
    support.attributes.push_back({
        "device_pitch_block",
        block,
        evidence_status::derived,
        1.0,
        "device_native",
    });
    return graph.add_edge(std::move(support));
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}
} // namespace

int main() {
    musical_execution_graph graph;

    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    const node_id episode_id = graph.add_node(std::move(episode));

    node parameter;
    parameter.kind = node_kind::parameter;
    parameter.layer = semantic_layer::musical_performance;
    parameter.flow = flow_kind::control;
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

    // Two register-derived pitch states occur without an intervening wait. The
    // raw graph retains both, but musical pitch motion should see only the final
    // state at source tick 10.
    const node_id high_half = add_transition(graph, 10);
    const node_id final_pair = add_transition(graph, 10);
    const node_id next_pitch = add_transition(graph, 20);
    add_pitch_support(graph, high_half, parameter_id, 10, 0x430, 5);
    add_pitch_support(graph, final_pair, parameter_id, 10, 0x440, 5);
    add_pitch_support(graph, next_pitch, parameter_id, 20, 0x450, 5);

    genesis_pitch_clock_context clocks;
    clocks.ym2612_clock_hz = 7670454;
    clocks.sn76489_clock_hz = 3579545;
    clocks.source = "fixture-vgm-header";

    const auto projection = project_genesis_pitch_motion(graph, parameter_id, clocks);
    CHECK(projection.raw_pitch_support_count == 3);
    CHECK(projection.same_tick_states_coalesced == 1);
    CHECK(projection.samples.size() == 2);
    CHECK(projection.samples[0].time.tick == 10);
    CHECK(projection.samples[0].source_node == final_pair);
    CHECK(projection.samples[1].time.tick == 20);
    CHECK(projection.samples[1].source_node == next_pitch);
    CHECK(projection.samples[0].physical_episode_id == episode_id);
    CHECK(projection.samples[0].pitch_basis == "absolute_nominal_device_frequency_hz");

    const auto expected = ym2612_nominal_pitch_frequency_hz(0x440, 5, clocks.ym2612_clock_hz);
    CHECK(expected.has_value());
    CHECK(close_enough(projection.samples[0].log2_pitch_coordinate, std::log2(*expected)));

    return 0;
}
