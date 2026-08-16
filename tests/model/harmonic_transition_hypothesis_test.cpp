#include "model/harmonic_transition_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_event(musical_execution_graph& graph, std::int64_t tick, const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{{time_domain::musical, tick, 960, 0}, std::nullopt};
    return graph.add_node(std::move(value));
}

tertian_triad_hypothesis chord(
    std::int64_t root,
    tertian_triad_quality quality,
    std::vector<std::int64_t> pitch_classes,
    std::int64_t tick,
    double confidence,
    std::vector<node_id> sources,
    bool ambiguous = false) {
    tertian_triad_hypothesis result;
    result.root_pitch_class = root;
    result.quality = quality;
    result.inversion = triad_inversion::root_position;
    result.confidence = confidence;
    result.root_ambiguous = ambiguous;
    result.pitch_classes = std::move(pitch_classes);
    result.projection.tuning.divisions_per_octave = 12;
    result.projection.tuning.reference_frequency_hz = 440.0;
    result.projection.tuning.reference_step = 69;
    result.projection.tuning.confidence = 1.0;
    result.projection.tuning.source = "explicit-12tet-control";
    result.projection.source_verticality.observation_time = {
        time_domain::musical, tick, 960, 0};
    result.projection.source_verticality.source_nodes = std::move(sources);
    return result;
}

const attribute* find_attribute(const edge& value, const char* name) {
    for (const auto& item : value.attributes) {
        if (item.name == name)
            return &item;
    }
    return nullptr;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const std::vector<node_id> a_sources{
        add_event(graph, 0, "A"),
        add_event(graph, 0, "C sharp"),
        add_event(graph, 0, "E"),
    };
    const std::vector<node_id> d_sources{
        add_event(graph, 480, "D"),
        add_event(graph, 480, "F sharp"),
        add_event(graph, 480, "A upper"),
    };

    const auto a_major = chord(
        9,
        tertian_triad_quality::major,
        {1, 4, 9},
        0,
        0.92,
        a_sources);
    const auto d_major = chord(
        2,
        tertian_triad_quality::major,
        {2, 6, 9},
        480,
        0.88,
        d_sources);

    const auto transition = infer_harmonic_transition(a_major, d_major);
    assert(transition.directed_root_motion_semitones == 5);
    assert(transition.root_interval_class == 5);
    assert(!transition.quality_changed);
    assert(transition.common_pitch_classes == 1);
    assert(transition.root_motion_reliable);
    assert(close_enough(transition.confidence, 0.88));

    const node_id a_node = add_tertian_triad_hypothesis(graph, a_major);
    const node_id d_node = add_tertian_triad_hypothesis(graph, d_major);
    const edge_id relation_id = add_harmonic_transition_hypothesis(
        graph,
        a_node,
        d_node,
        transition);
    const edge* relation = graph.find_edge(relation_id);
    assert(relation != nullptr);
    assert(relation->kind == edge_kind::transforms);
    assert(std::get<std::int64_t>(
        find_attribute(*relation, "directed_root_motion_semitones")->value) == 5);
    assert(std::get<std::uint64_t>(
        find_attribute(*relation, "common_pitch_classes")->value) == 1);

    // Tonal function is intentionally absent. Root-class motion is observable
    // here, but IV/V/I language requires a separately grounded key/context.
    assert(find_attribute(*relation, "roman_numeral") == nullptr);
    assert(find_attribute(*relation, "harmonic_function") == nullptr);
    assert(find_attribute(*relation, "cadence") == nullptr);
    assert(find_attribute(*relation, "key") == nullptr);

    // A symmetrical/root-ambiguous chord cannot yield highly confident root
    // motion even if its pitch-class collection itself was strong.
    const auto ambiguous = chord(
        0,
        tertian_triad_quality::augmented,
        {0, 4, 8},
        960,
        0.91,
        {
            add_event(graph, 960, "C"),
            add_event(graph, 960, "E2"),
            add_event(graph, 960, "G sharp"),
        },
        true);
    const auto ambiguous_transition = infer_harmonic_transition(d_major, ambiguous);
    assert(!ambiguous_transition.root_motion_reliable);
    assert(close_enough(
        ambiguous_transition.confidence,
        ambiguous_root_transition_ceiling));

    // Temporal order is part of the transition claim.
    auto reversed = a_major;
    reversed.projection.source_verticality.observation_time.tick = 240;
    bool rejected_reversed = false;
    try {
        (void)infer_harmonic_transition(d_major, reversed);
    } catch (const std::invalid_argument&) {
        rejected_reversed = true;
    }
    assert(rejected_reversed);

    return 0;
}
