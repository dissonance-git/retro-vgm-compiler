#include "components/spc/spc_label_blind_corpus_features.h"
#include "components/spc/spc_persistent_performance_adapter.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

namespace {

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    std::uint64_t voice = 0) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = "S-DSP physical voice episode";
    value.active = time_span{
        {time_domain::device, start, 32000, 0},
        time_coordinate{time_domain::device, end, 32000, 0},
    };
    value.attributes.push_back({"physical_voice", voice, evidence_status::derived, 1.0, "slot"});
    return graph.add_node(std::move(value));
}

node_id add_sample(musical_execution_graph& graph, const char* label = "BRR runtime version") {
    node value;
    value.kind = node_kind::sample_buffer;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::value;
    value.label = label;
    value.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    return graph.add_node(std::move(value));
}

node_id add_runtime_event(
    musical_execution_graph& graph,
    node_id episode,
    node_id sample,
    std::int64_t tick,
    std::uint64_t pitch_rate,
    const char* kind,
    bool include_pitch = true) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = std::string{"S-DSP "} + kind;
    event.active = time_span{{time_domain::device, tick, 32000, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{kind}, evidence_status::exact, 1.0, ""});
    event.attributes.push_back({"physical_voice", std::uint64_t{0}, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"source_index", std::uint64_t{7}, evidence_status::exact, 1.0, "slot"});
    if (include_pitch)
        event.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    const node_id event_id = graph.add_node(std::move(event));

    edge relation;
    relation.kind = std::string{kind} == "key_on_accepted"
        ? edge_kind::causes
        : edge_kind::contributes_to;
    relation.from = event_id;
    relation.to = episode;
    graph.add_edge(std::move(relation));

    edge reference;
    reference.kind = edge_kind::references;
    reference.from = event_id;
    reference.to = sample;
    graph.add_edge(std::move(reference));
    return event_id;
}

struct fixture {
    musical_execution_graph graph;
    node_id sample = 0;
    node_id first = 0;
    node_id second = 0;
    node_id third = 0;
};

fixture make_fixture(bool zero_final_pitch = false, bool continuity_barrier = false) {
    fixture result;
    result.sample = add_sample(result.graph);
    result.first = add_episode(result.graph, 0, 3200);
    result.second = add_episode(result.graph, 3520, 6400);
    result.third = add_episode(result.graph, 6720, 9600);

    add_runtime_event(result.graph, result.first, result.sample, 0, 0x1000, "key_on_accepted");
    add_runtime_event(result.graph, result.second, result.sample, 3520, 0x1200, "key_on_accepted");
    add_runtime_event(
        result.graph,
        result.third,
        result.sample,
        6720,
        zero_final_pitch ? 0 : 0x0f00,
        "key_on_accepted");

    // A source observation inside one physical episode remains part of that
    // episode. Referencing the same event-time BRR version cannot create a fake
    // retrigger or an extra performance boundary.
    add_runtime_event(
        result.graph,
        result.second,
        result.sample,
        4000,
        0,
        "source_latched",
        false);

    if (continuity_barrier) {
        node* second = result.graph.find_node(result.second);
        assert(second != nullptr);
        second->attributes.push_back({
            "termination_reason",
            std::string{"semantic_continuation_lost"},
            evidence_status::derived,
            1.0,
            "",
        });
        second->attributes.push_back({
            "termination_boundary_complete",
            false,
            evidence_status::derived,
            1.0,
            "",
        });
    }
    return result;
}

spc_part_continuity_policy policy() {
    spc_part_continuity_policy value;
    value.max_gap_seconds = 0.02;
    value.max_pitch_interval_octaves = 1.0;
    return value;
}

} // namespace

int main() {
    {
        auto value = make_fixture();
        const auto corpus = extract_spc_label_blind_corpus_features(
            value.graph,
            "spc-persistent-performance-test",
            policy());
        assert(corpus.emitted_part_count == 1);

        const auto performances = discover_spc_persistent_performances(
            value.graph,
            "spc-persistent-performance-test",
            policy());
        assert(performances.size() == 1);
        const auto& performance = performances.front();
        assert(performance.identity.subject_nodes.size() == 3);
        assert(performance.identity.subject_nodes[0] == value.first);
        assert(performance.identity.subject_nodes[1] == value.second);
        assert(performance.identity.subject_nodes[2] == value.third);
        assert(performance.segments.size() == 3);
        assert(performance.rearticulation_boundaries.size() == 2);
        assert(performance.confidence >= persistent_part_trajectory_link_threshold);

        for (const auto& segment : performance.segments) {
            assert(segment.samples.size() == 1);
            assert(segment.articulation.kind ==
                   pitch_motion_articulation_kind::in_episode_pitch_change_unresolved);
            assert(segment.samples.front().pitch_basis ==
                   "snes_sdsp_pitch_rate_relative_to_event_time_brr_source");
            assert(segment.samples.front().interval_semantics ==
                   "log2_playback_rate_ratio_octaves");
        }

        const double first_to_second =
            performance.segments[1].samples[0].log2_pitch_coordinate -
            performance.segments[0].samples[0].log2_pitch_coordinate;
        assert(std::fabs(first_to_second - std::log2(1.125)) < 1e-12);

        assert(performance.rearticulation_boundaries[0].physical_episode_id == value.first);
        assert(performance.rearticulation_boundaries[0].next_physical_episode_id == value.second);
        assert(performance.rearticulation_boundaries[1].physical_episode_id == value.second);
        assert(performance.rearticulation_boundaries[1].next_physical_episode_id == value.third);
    }

    {
        auto value = make_fixture(true, false);
        const auto corpus = extract_spc_label_blind_corpus_features(
            value.graph,
            "zero-pitch-test",
            policy());
        assert(corpus.emitted_part_count == 1);

        // Identity can remain strong without a pitch observation, but the
        // performed trajectory cannot. Missing/zero S-DSP pitch fails closed.
        const auto performances = discover_spc_persistent_performances(
            value.graph,
            "zero-pitch-test",
            policy());
        assert(performances.empty());
    }

    {
        auto value = make_fixture(false, true);
        const auto corpus = extract_spc_label_blind_corpus_features(
            value.graph,
            "continuation-loss-test",
            policy());
        assert(corpus.continuity_barrier_count == 1);
        assert(corpus.emitted_part_count == 0);
        assert(discover_spc_persistent_performances(
            value.graph,
            "continuation-loss-test",
            policy()).empty());
    }

    {
        auto value = make_fixture();
        const node_id changed_sample = add_sample(value.graph, "different BRR runtime version");
        add_runtime_event(
            value.graph,
            value.second,
            changed_sample,
            4100,
            0,
            "source_latched",
            false);

        const auto corpus = extract_spc_label_blind_corpus_features(
            value.graph,
            "source-mutation-test",
            policy());
        assert(corpus.emitted_part_count == 0);
        assert(discover_spc_persistent_performances(
            value.graph,
            "source-mutation-test",
            policy()).empty());
    }

    return 0;
}
