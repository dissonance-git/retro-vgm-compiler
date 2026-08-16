#include "components/spc/spc_orchestration_adapter.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;
using namespace gameaudio::spc;

namespace {

time_span span(std::int64_t begin, std::int64_t end) {
    return time_span{
        time_coordinate{time_domain::device, begin, 32000, 0},
        time_coordinate{time_domain::device, end, 32000, 0},
    };
}

node_id add_part(musical_execution_graph& graph) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        0.90,
        "",
    });
    return graph.add_node(std::move(part));
}

node_id add_sample(musical_execution_graph& graph) {
    node sample;
    sample.kind = node_kind::sample_buffer;
    sample.layer = semantic_layer::synthesis;
    sample.flow = flow_kind::value;
    sample.label = "event-time BRR sample version";
    return graph.add_node(std::move(sample));
}

node_id add_episode(
    musical_execution_graph& graph,
    node_id part_id,
    node_id sample_id,
    std::int64_t begin,
    std::int64_t end,
    std::uint64_t pitch_rate) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.active = span(begin, end);
    const node_id episode_id = graph.add_node(std::move(episode));

    edge membership;
    membership.kind = edge_kind::groups_into;
    membership.from = episode_id;
    membership.to = part_id;
    graph.add_edge(std::move(membership));

    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.attributes.push_back({
        "event_kind",
        std::string{"key_on_accepted"},
        evidence_status::derived,
        1.0,
        "",
    });
    event.attributes.push_back({
        "pitch_rate",
        pitch_rate,
        evidence_status::exact,
        1.0,
        "s-dsp pitch rate",
    });
    const node_id event_id = graph.add_node(std::move(event));

    edge causes;
    causes.kind = edge_kind::causes;
    causes.from = event_id;
    causes.to = episode_id;
    graph.add_edge(std::move(causes));

    edge references;
    references.kind = edge_kind::references;
    references.from = event_id;
    references.to = sample_id;
    graph.add_edge(std::move(references));
    return episode_id;
}

musical_part_role_hypothesis foreground_role(node_id part_id) {
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        span(0, 200),
        0.84,
        {
            {
                part_role_evidence_kind::melodic_motif_prominence,
                part_role_evidence_origin::musical_analysis,
                part_role_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.86,
                "spc-role-test",
                "persistent part carries the principal motif",
                {part_id},
            },
            {
                part_role_evidence_kind::auditory_salience,
                part_role_evidence_origin::auditory_analysis,
                part_role_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.84,
                "spc-role-test",
                "independent salience support",
                {part_id},
            },
        });
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id part_id = add_part(graph);
    const node_id sample_id = add_sample(graph);
    add_episode(graph, part_id, sample_id, 0, 80, 0x1000);
    add_episode(graph, part_id, sample_id, 100, 180, 0x2000);

    const auto role = foreground_role(part_id);
    const auto summary = summarize_spc_part_orchestration(graph, role);
    assert(summary.episode_ids.size() == 2);
    assert(summary.sample_version_ids.size() == 1);
    assert(summary.all_relevant_episodes_have_one_sample_version);
    assert(summary.register_center_log2_pitch_rate.has_value());
    assert(std::fabs(*summary.register_center_log2_pitch_rate - 12.5) < 1e-12);

    const auto state = make_spc_part_orchestration_state(
        graph,
        role,
        "synthetic-spc");
    assert(state.realization.has_value());
    assert(state.realization->basis == "spc_brr_sample_version_node");
    assert(state.realization->identity == std::to_string(sample_id));
    assert(state.register_coordinate.has_value());
    assert(state.register_basis == "spc_same_sample_log2_pitch_rate");

    // Different exact BRR versions in one role span are not one timbre identity,
    // and their pitch-rate registers cannot be compared as one register basis
    // without a separate root-tuning correspondence.
    musical_execution_graph mixed_graph;
    const node_id mixed_part = add_part(mixed_graph);
    const node_id sample_a = add_sample(mixed_graph);
    const node_id sample_b = add_sample(mixed_graph);
    add_episode(mixed_graph, mixed_part, sample_a, 0, 80, 0x1000);
    add_episode(mixed_graph, mixed_part, sample_b, 100, 180, 0x2000);
    const auto mixed_role = foreground_role(mixed_part);
    const auto mixed_state = make_spc_part_orchestration_state(
        mixed_graph,
        mixed_role,
        "synthetic-spc");
    assert(!mixed_state.realization.has_value());
    assert(!mixed_state.register_coordinate.has_value());
    assert(mixed_state.register_basis.empty());

    return 0;
}
