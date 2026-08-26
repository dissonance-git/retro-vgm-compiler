#include "model/harmonic_verticality.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_part(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::part;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = label;
    return graph.add_node(std::move(value));
}

node_id add_pitch_event(musical_execution_graph& graph, std::int64_t start, std::int64_t end) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = "absolute musical pitch observation";
    value.active = time_span{
        {time_domain::authored, start, 960, 0},
        time_coordinate{time_domain::authored, end, 960, 0},
    };
    return graph.add_node(std::move(value));
}

absolute_musical_pitch_observation pitch(
    node_id source,
    node_id part,
    std::int64_t start,
    std::int64_t end,
    double frequency,
    musical_pitch_role role,
    double confidence) {
    return {
        source,
        part,
        {
            {time_domain::authored, start, 960, 0},
            time_coordinate{time_domain::authored, end, 960, 0},
        },
        frequency,
        role,
        evidence_status::derived,
        confidence,
        "synthetic-absolute-pitch",
    };
}

const attribute* find_attribute(const std::vector<attribute>& attributes, const char* name) {
    for (const auto& item : attributes) {
        if (item.key == name)
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
    const node_id bass = add_part(graph, "bass");
    const node_id inner = add_part(graph, "inner voice");
    const node_id melody = add_part(graph, "melody");

    const node_id bass_event = add_pitch_event(graph, 0, 960);
    const node_id inner_event = add_pitch_event(graph, 0, 960);
    const node_id melody_event = add_pitch_event(graph, 0, 960);

    const time_coordinate sample_time{time_domain::authored, 480, 960, 0};
    const auto verticality = make_harmonic_verticality(
        sample_time,
        {
            pitch(bass_event, bass, 0, 960, 110.0, musical_pitch_role::programmed, 0.98),
            pitch(inner_event, inner, 0, 960, 137.5, musical_pitch_role::programmed, 0.94),
            pitch(melody_event, melody, 0, 960, 165.0, musical_pitch_role::programmed, 0.91),
        });

    assert(verticality.frequencies_hz.size() == 3);
    assert(verticality.part_ids.size() == 3);
    assert(close_enough(verticality.confidence, 0.91));
    assert(close_enough(verticality.intervals_above_lowest_octaves[0], 0.0));
    assert(close_enough(
        verticality.intervals_above_lowest_octaves[1],
        std::log2(137.5 / 110.0)));
    assert(close_enough(
        verticality.intervals_above_lowest_octaves[2],
        std::log2(165.0 / 110.0)));

    const node_id collection = add_harmonic_verticality(graph, verticality);
    const node* materialized = graph.find_node(collection);
    assert(materialized != nullptr);
    assert(materialized->kind == node_kind::pattern);
    assert(materialized->layer == semantic_layer::musical_structure);

    const attribute* scope = find_attribute(materialized->attributes, "identity_scope");
    assert(scope != nullptr);
    assert(std::get<std::string>(scope->value) == "harmonic_verticality");

    assert(find_attribute(materialized->attributes, "chord") == nullptr);
    assert(find_attribute(materialized->attributes, "root") == nullptr);
    assert(find_attribute(materialized->attributes, "key") == nullptr);
    assert(find_attribute(materialized->attributes, "harmonic_function") == nullptr);

    const auto supports = graph.edges_to(collection, edge_kind::derived_from);
    assert(supports.size() == 3);
    for (const edge* support : supports) {
        assert(find_attribute(support->attributes, "frequency_hz") != nullptr);
        assert(find_attribute(support->attributes, "interval_above_lowest_octaves") != nullptr);
        assert(find_attribute(support->attributes, "persistent_part_id") != nullptr);
    }

    bool rejected_mixed_roles = false;
    try {
        (void)make_harmonic_verticality(
            sample_time,
            {
                pitch(bass_event, bass, 0, 960, 110.0, musical_pitch_role::programmed, 0.98),
                pitch(inner_event, inner, 0, 960, 137.5, musical_pitch_role::heard, 0.94),
            });
    } catch (const std::invalid_argument&) {
        rejected_mixed_roles = true;
    }
    assert(rejected_mixed_roles);

    bool rejected_inactive = false;
    try {
        (void)make_harmonic_verticality(
            sample_time,
            {
                pitch(bass_event, bass, 0, 960, 110.0, musical_pitch_role::performed, 0.98),
                pitch(inner_event, inner, 960, 1920, 137.5, musical_pitch_role::performed, 0.94),
            });
    } catch (const std::invalid_argument&) {
        rejected_inactive = true;
    }
    assert(rejected_inactive);

    bool rejected_nonpositive = false;
    try {
        auto invalid = pitch(
            bass_event,
            bass,
            0,
            960,
            0.0,
            musical_pitch_role::programmed,
            0.98);
        (void)make_harmonic_verticality(sample_time, {invalid, invalid});
    } catch (const std::invalid_argument&) {
        rejected_nonpositive = true;
    }
    assert(rejected_nonpositive);

    return 0;
}
