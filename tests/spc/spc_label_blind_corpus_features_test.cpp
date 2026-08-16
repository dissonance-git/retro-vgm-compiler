#include "components/spc/spc_label_blind_corpus_features.h"

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
    std::uint64_t voice) {
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

node_id add_sample(musical_execution_graph& graph) {
    node value;
    value.kind = node_kind::sample_buffer;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::value;
    value.label = "BRR runtime version";
    value.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    return graph.add_node(std::move(value));
}

void add_key_on(
    musical_execution_graph& graph,
    node_id episode,
    node_id sample,
    std::int64_t tick,
    std::uint64_t voice,
    std::uint64_t pitch_rate) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = "S-DSP key_on_accepted";
    event.active = time_span{{time_domain::device, tick, 32000, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{"key_on_accepted"}, evidence_status::exact, 1.0, ""});
    event.attributes.push_back({"physical_voice", voice, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"source_index", std::uint64_t{7}, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    const node_id event_id = graph.add_node(std::move(event));

    edge cause;
    cause.kind = edge_kind::causes;
    cause.from = event_id;
    cause.to = episode;
    graph.add_edge(std::move(cause));

    edge reference;
    reference.kind = edge_kind::references;
    reference.from = event_id;
    reference.to = sample;
    graph.add_edge(std::move(reference));
}

musical_execution_graph make_runtime_graph(
    bool poison_with_metadata,
    bool insert_continuity_barrier = false) {
    musical_execution_graph graph;
    const node_id sample = add_sample(graph);

    const node_id first = add_episode(graph, 0, 3200, 0);
    const node_id second = add_episode(graph, 3520, 6400, 0);
    const node_id third = add_episode(graph, 6720, 9600, 0);
    add_key_on(graph, first, sample, 0, 0, 0x1000);
    add_key_on(graph, second, sample, 3520, 0, 0x1200);
    add_key_on(graph, third, sample, 6720, 0, 0x0f00);

    if (insert_continuity_barrier) {
        node* episode = graph.find_node(second);
        assert(episode != nullptr);
        episode->attributes.push_back({
            "termination_reason",
            std::string{"semantic_continuation_lost"},
            evidence_status::derived,
            1.0,
            "",
        });
        episode->attributes.push_back({
            "termination_boundary_complete",
            false,
            evidence_status::derived,
            1.0,
            "",
        });
    }

    // A cross-slot handoff can be musically real, but this conservative corpus
    // pass must not invent it. These isolated episodes share the sample yet are
    // intentionally left outside the slot-anchored trajectory above.
    const node_id cross_slot_a = add_episode(graph, 10000, 11200, 1);
    const node_id cross_slot_b = add_episode(graph, 11520, 12800, 2);
    add_key_on(graph, cross_slot_a, sample, 10000, 1, 0x1000);
    add_key_on(graph, cross_slot_b, sample, 11520, 2, 0x1100);

    if (poison_with_metadata) {
        node metadata;
        metadata.kind = node_kind::source_object;
        metadata.layer = semantic_layer::source_representation;
        metadata.flow = flow_kind::value;
        metadata.label = "metadata poison pill";
        metadata.attributes.push_back({"title", std::string{"THIS TITLE MUST NOT LEAK"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"game", std::string{"WRONG GAME"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"artist", std::string{"WRONG ARTIST"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"composer", std::string{"WRONG COMPOSER"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"gd3_artist", std::string{"STALE GD3"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"id666_artist", std::string{"STALE ID666"}, evidence_status::exact, 1.0, ""});
        metadata.attributes.push_back({"external_artist", std::string{"ROUTING LABEL ONLY"}, evidence_status::exact, 1.0, ""});
        graph.add_node(std::move(metadata));
    }

    return graph;
}

bool same_optional_doubles(
    const std::optional<std::vector<double>>& lhs,
    const std::optional<std::vector<double>>& rhs) {
    if (lhs.has_value() != rhs.has_value())
        return false;
    if (!lhs.has_value())
        return true;
    if (lhs->size() != rhs->size())
        return false;
    for (std::size_t i = 0; i < lhs->size(); ++i) {
        if (std::fabs((*lhs)[i] - (*rhs)[i]) > 1e-12)
            return false;
    }
    return true;
}

bool same_geometry(const part_motif_profile& lhs, const part_motif_profile& rhs) {
    if (lhs.normalized_inter_onset_intervals.size() != rhs.normalized_inter_onset_intervals.size())
        return false;
    for (std::size_t i = 0; i < lhs.normalized_inter_onset_intervals.size(); ++i) {
        if (std::fabs(lhs.normalized_inter_onset_intervals[i] - rhs.normalized_inter_onset_intervals[i]) > 1e-12)
            return false;
    }
    return same_optional_doubles(lhs.interval_octaves, rhs.interval_octaves) &&
           lhs.pitch_contour == rhs.pitch_contour &&
           lhs.pitch_basis == rhs.pitch_basis &&
           lhs.interval_semantics == rhs.interval_semantics &&
           lhs.pitch_range_octaves == rhs.pitch_range_octaves &&
           lhs.status == rhs.status &&
           std::fabs(lhs.evidence_confidence - rhs.evidence_confidence) <= 1e-12;
}

} // namespace

int main() {
    spc_part_continuity_policy policy;
    policy.max_gap_seconds = 0.02;
    policy.max_pitch_interval_octaves = 1.0;

    auto clean_graph = make_runtime_graph(false);
    const auto clean = extract_spc_label_blind_corpus_features(
        clean_graph,
        "synthetic-runtime-trace",
        policy);

    assert(clean.voice_episode_count == 5);
    assert(clean.eligible_episode_count == 5);
    assert(clean.candidate_transition_count == 2);
    assert(clean.strong_transition_count == 2);
    assert(clean.rejected_transition_count == 0);
    assert(clean.continuity_barrier_count == 0);
    assert(clean.emitted_part_count == 1);
    assert(clean.part_profiles.size() == 1);
    assert(clean.part_profiles.front().source_nodes.size() == 3);
    assert(clean.part_profiles.front().normalized_inter_onset_intervals.size() == 2);
    assert(clean.part_profiles.front().interval_octaves.has_value());

    auto poisoned_graph = make_runtime_graph(true);
    const auto poisoned = extract_spc_label_blind_corpus_features(
        poisoned_graph,
        "synthetic-runtime-trace",
        policy);

    // Catalog metadata, including stale embedded tags and even the authoritative
    // external routing label, is invisible to musical feature extraction.
    assert(poisoned.voice_episode_count == clean.voice_episode_count);
    assert(poisoned.eligible_episode_count == clean.eligible_episode_count);
    assert(poisoned.candidate_transition_count == clean.candidate_transition_count);
    assert(poisoned.strong_transition_count == clean.strong_transition_count);
    assert(poisoned.rejected_transition_count == clean.rejected_transition_count);
    assert(poisoned.continuity_barrier_count == clean.continuity_barrier_count);
    assert(poisoned.emitted_part_count == clean.emitted_part_count);
    assert(poisoned.part_profiles.size() == clean.part_profiles.size());
    assert(same_geometry(poisoned.part_profiles.front(), clean.part_profiles.front()));

    auto broken_graph = make_runtime_graph(false, true);
    const auto broken = extract_spc_label_blind_corpus_features(
        broken_graph,
        "synthetic-runtime-trace",
        policy);
    assert(broken.continuity_barrier_count == 1);
    assert(broken.candidate_transition_count == 1);
    assert(broken.strong_transition_count == 1);
    assert(broken.emitted_part_count == 0);
    assert(broken.part_profiles.empty());

    return 0;
}
