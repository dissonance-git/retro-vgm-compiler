#include "components/spc/spc_part_motif_adapter.h"
#include "components/vgm/enhancement/genesis_part_motif_adapter.h"
#include "model/persistent_part_hypothesis.h"

#include <algorithm>
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
    return add_persistent_part_hypothesis(
        graph,
        make_persistent_part_hypothesis(
            0.92,
            episodes,
            {
                {
                    persistent_part_evidence_kind::source_identity,
                    persistent_part_evidence_origin::synthesis_runtime,
                    persistent_part_evidence_polarity::supports,
                    evidence_status::derived,
                    0.95,
                    source,
                    "synthetic source continuity",
                    episodes,
                },
                {
                    persistent_part_evidence_kind::temporal_adjacency,
                    persistent_part_evidence_origin::musical_analysis,
                    persistent_part_evidence_polarity::supports,
                    evidence_status::derived,
                    0.90,
                    source,
                    "synthetic line continuity",
                    episodes,
                },
            }));
}

node_id add_genesis_episode(
    musical_execution_graph& graph,
    std::int64_t tick,
    std::uint16_t fnum,
    std::uint8_t block) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{
        {time_domain::source, tick, 0, 0},
        time_coordinate{time_domain::source, tick + 70, 0, 0},
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
    onset.attributes.push_back({"device_pitch_block", static_cast<std::uint64_t>(block), evidence_status::derived, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realizes;
    realizes.kind = edge_kind::realizes;
    realizes.from = onset_id;
    realizes.to = episode_id;
    graph.add_edge(std::move(realizes));
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
    node_id sample,
    std::int64_t tick,
    std::uint64_t pitch_rate) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "S-DSP physical voice episode";
    episode.active = time_span{
        {time_domain::device, tick, 32000, 0},
        time_coordinate{time_domain::device, tick + 140, 32000, 0},
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
    onset.attributes.push_back({"source_index", std::uint64_t{3}, evidence_status::exact, 1.0, "slot"});
    onset.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge causes;
    causes.kind = edge_kind::causes;
    causes.from = onset_id;
    causes.to = episode_id;
    graph.add_edge(std::move(causes));

    edge references;
    references.kind = edge_kind::references;
    references.from = onset_id;
    references.to = sample;
    graph.add_edge(std::move(references));
    return episode_id;
}

const phrase_boundary_hypothesis* strongest_boundary(
    const std::vector<phrase_boundary_hypothesis>& hypotheses) {
    if (hypotheses.empty())
        return nullptr;
    return &*std::max_element(
        hypotheses.begin(),
        hypotheses.end(),
        [](const auto& first, const auto& second) {
            return first.confidence < second.confidence;
        });
}

} // namespace

int main() {
    part_motif_discovery_policy motif_policy;
    motif_policy.min_events = 4;
    motif_policy.max_events = 4;
    motif_policy.min_identity_confidence = 0.99;

    musical_execution_graph genesis;
    const std::vector<node_id> genesis_episodes = {
        add_genesis_episode(genesis, 0, 0x300, 3),
        add_genesis_episode(genesis, 100, 0x360, 3),
        add_genesis_episode(genesis, 200, 0x3c0, 3),
        add_genesis_episode(genesis, 400, 0x330, 3),
        add_genesis_episode(genesis, 1000, 0x300, 4),
        add_genesis_episode(genesis, 1200, 0x360, 4),
        add_genesis_episode(genesis, 1400, 0x3c0, 4),
        add_genesis_episode(genesis, 1800, 0x330, 4),
    };
    const node_id genesis_part = add_part(genesis, genesis_episodes, "genesis-control");
    const auto genesis_phrases = gameaudio::vgm::discover_genesis_part_phrase_boundaries(
        genesis,
        genesis_part,
        motif_policy,
        2.0);
    const auto* genesis_strong = strongest_boundary(genesis_phrases);
    assert(genesis_strong != nullptr);
    assert(genesis_strong->boundary.tick == 1000);
    assert(genesis_strong->structural_support);
    assert(genesis_strong->cross_domain_grounded);
    assert(genesis_strong->confidence >= 0.80);

    musical_execution_graph spc;
    const node_id sample = add_spc_sample(spc);
    const std::vector<node_id> spc_episodes = {
        add_spc_episode(spc, sample, 0, 4096),
        add_spc_episode(spc, sample, 200, 4608),
        add_spc_episode(spc, sample, 400, 5120),
        add_spc_episode(spc, sample, 800, 4352),
        add_spc_episode(spc, sample, 2000, 8192),
        add_spc_episode(spc, sample, 2400, 9216),
        add_spc_episode(spc, sample, 2800, 10240),
        add_spc_episode(spc, sample, 3600, 8704),
    };
    const node_id spc_part = add_part(spc, spc_episodes, "spc-control");
    const auto spc_phrases = gameaudio::spc::discover_spc_part_phrase_boundaries(
        spc,
        spc_part,
        motif_policy,
        2.0);
    const auto* spc_strong = strongest_boundary(spc_phrases);
    assert(spc_strong != nullptr);
    assert(spc_strong->boundary.tick == 2000);
    assert(spc_strong->structural_support);
    assert(spc_strong->cross_domain_grounded);
    assert(spc_strong->confidence >= 0.80);

    // The physical clocks differ by 2x, but both systems recover the same
    // normalized phrase location: after four events and before the repeated,
    // transposed motif occurrence.
    assert(std::fabs(genesis_strong->confidence - spc_strong->confidence) < 1e-12);

    return 0;
}
