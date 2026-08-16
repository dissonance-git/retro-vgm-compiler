#include "components/spc/spc_part_motif_adapter.h"
#include "components/vgm/enhancement/genesis_part_motif_adapter.h"
#include "model/persistent_part_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_part(
    musical_execution_graph& graph,
    const std::vector<node_id>& episodes,
    const char* source) {
    std::vector<persistent_part_evidence> evidence;
    evidence.push_back({
        persistent_part_evidence_kind::source_identity,
        persistent_part_evidence_origin::synthesis_runtime,
        persistent_part_evidence_polarity::supports,
        evidence_status::derived,
        0.95,
        source,
        "synthetic source/program continuity across the bounded episodes",
        episodes,
    });
    evidence.push_back({
        persistent_part_evidence_kind::temporal_adjacency,
        persistent_part_evidence_origin::musical_analysis,
        persistent_part_evidence_polarity::supports,
        evidence_status::derived,
        0.90,
        source,
        "synthetic successive onsets form one bounded musical line",
        episodes,
    });
    return add_persistent_part_hypothesis(
        graph,
        make_persistent_part_hypothesis(0.90, episodes, std::move(evidence)));
}

node_id add_genesis_episode(
    musical_execution_graph& graph,
    std::int64_t tick,
    std::uint16_t fnum) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{
        {time_domain::source, tick, 0, 0},
        time_coordinate{time_domain::source, tick + 80, 0, 0},
    };
    episode.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({"instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({"physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    const node_id episode_id = graph.add_node(std::move(episode));

    node onset;
    onset.kind = node_kind::musical_event;
    onset.layer = semantic_layer::musical_performance;
    onset.flow = flow_kind::event;
    onset.label = "YM2612 pitched activity onset";
    onset.active = time_span{{time_domain::source, tick, 0, 0}, std::nullopt};
    onset.attributes.push_back({"event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({"device_pitch_code", static_cast<std::uint64_t>(fnum), evidence_status::derived, 1.0, "device_native"});
    onset.attributes.push_back({"device_pitch_block", std::uint64_t{3}, evidence_status::derived, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realization;
    realization.kind = edge_kind::realizes;
    realization.from = onset_id;
    realization.to = episode_id;
    graph.add_edge(std::move(realization));
    return episode_id;
}

node_id add_spc_sample(musical_execution_graph& graph) {
    node sample;
    sample.kind = node_kind::sample_buffer;
    sample.layer = semantic_layer::synthesis;
    sample.flow = flow_kind::value;
    sample.label = "BRR runtime sample version";
    sample.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    return graph.add_node(std::move(sample));
}

node_id add_spc_episode(
    musical_execution_graph& graph,
    node_id sample_id,
    std::int64_t tick,
    std::uint64_t pitch_rate) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "S-DSP physical voice episode";
    episode.active = time_span{
        {time_domain::device, tick, 32000, 0},
        time_coordinate{time_domain::device, tick + 80, 32000, 0},
    };
    episode.attributes.push_back({"physical_voice", std::uint64_t{0}, evidence_status::derived, 1.0, "slot"});
    const node_id episode_id = graph.add_node(std::move(episode));

    node onset;
    onset.kind = node_kind::trace_event;
    onset.layer = semantic_layer::synthesis;
    onset.flow = flow_kind::event;
    onset.label = "S-DSP key_on_accepted";
    onset.active = time_span{{time_domain::device, tick, 32000, 0}, std::nullopt};
    onset.attributes.push_back({"event_kind", std::string{"key_on_accepted"}, evidence_status::exact, 1.0, ""});
    onset.attributes.push_back({"physical_voice", std::uint64_t{0}, evidence_status::exact, 1.0, "slot"});
    onset.attributes.push_back({"source_index", std::uint64_t{7}, evidence_status::exact, 1.0, "slot"});
    onset.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge cause;
    cause.kind = edge_kind::causes;
    cause.from = onset_id;
    cause.to = episode_id;
    graph.add_edge(std::move(cause));

    edge sample_reference;
    sample_reference.kind = edge_kind::references;
    sample_reference.from = onset_id;
    sample_reference.to = sample_id;
    graph.add_edge(std::move(sample_reference));
    return episode_id;
}

} // namespace

int main() {
    musical_execution_graph genesis_graph;
    const std::vector<node_id> genesis_episodes = {
        add_genesis_episode(genesis_graph, 0, 0x300),
        add_genesis_episode(genesis_graph, 100, 0x360),
        add_genesis_episode(genesis_graph, 200, 0x3c0),
        add_genesis_episode(genesis_graph, 400, 0x330),
    };
    const node_id genesis_part = add_part(
        genesis_graph,
        genesis_episodes,
        "synthetic-genesis-line");
    const auto genesis_profile = gameaudio::vgm::make_genesis_part_motif_profile(
        genesis_graph,
        genesis_part);
    assert(genesis_profile.has_value());

    musical_execution_graph spc_graph;
    const node_id sample = add_spc_sample(spc_graph);
    const std::vector<node_id> spc_episodes = {
        add_spc_episode(spc_graph, sample, 0, 4096),
        add_spc_episode(spc_graph, sample, 200, 4608),
        add_spc_episode(spc_graph, sample, 400, 5120),
        add_spc_episode(spc_graph, sample, 800, 4352),
    };
    const node_id spc_part = add_part(
        spc_graph,
        spc_episodes,
        "synthetic-spc-line");
    const auto spc_profile = gameaudio::spc::make_spc_part_motif_profile(
        spc_graph,
        spc_part);
    assert(spc_profile.has_value());

    // Native pitch coordinate systems remain distinct.
    assert(genesis_profile->pitch_basis == "genesis_ym2612_relative_frequency_code");
    assert(spc_profile->pitch_basis == "spc_brr_runtime_version:" + std::to_string(sample));
    assert(genesis_profile->pitch_basis != spc_profile->pitch_basis);

    // Both adapters have independently earned the same interval semantics.
    assert(genesis_profile->interval_semantics == "log2_frequency_ratio_octaves");
    assert(spc_profile->interval_semantics == "log2_frequency_ratio_octaves");

    const auto similarity = compare_part_motif_profiles(*genesis_profile, *spc_profile);
    assert(similarity.pitch_comparable);
    assert(similarity.interval_similarity.has_value());
    assert(similarity.contour_similarity.has_value());
    assert(std::fabs(*similarity.interval_similarity - 1.0) < 1e-12);
    assert(std::fabs(similarity.rhythm_similarity - 1.0) < 1e-12);
    assert(std::fabs(*similarity.contour_similarity - 1.0) < 1e-12);
    assert(std::fabs(similarity.identity_confidence - 1.0) < 1e-12);

    return 0;
}
