#include "components/vgm/enhancement/genesis_orchestration_adapter.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;
using namespace gameaudio::vgm;

namespace {

time_span span(std::int64_t begin, std::int64_t end) {
    return time_span{
        time_coordinate{time_domain::source, begin, 44100, 0},
        time_coordinate{time_domain::source, end, 44100, 0},
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

node_id add_episode(
    musical_execution_graph& graph,
    node_id part_id,
    std::int64_t begin,
    std::int64_t end,
    std::uint64_t fingerprint,
    std::uint64_t fnum,
    std::uint64_t block) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.active = span(begin, end);
    episode.attributes.push_back({
        "device_family",
        std::string{"YM2612"},
        evidence_status::derived,
        1.0,
        "",
    });
    episode.attributes.push_back({
        "instrument_program_fingerprint",
        fingerprint,
        evidence_status::derived,
        1.0,
        "fnv1a64",
    });
    const node_id episode_id = graph.add_node(std::move(episode));

    edge membership;
    membership.kind = edge_kind::groups_into;
    membership.from = episode_id;
    membership.to = part_id;
    graph.add_edge(std::move(membership));

    node onset;
    onset.kind = node_kind::musical_event;
    onset.layer = semantic_layer::musical_performance;
    onset.flow = flow_kind::event;
    onset.active = time_span{time_coordinate{time_domain::source, begin, 44100, 0}, std::nullopt};
    onset.attributes.push_back({
        "event_kind",
        std::string{"pitched_activity_onset"},
        evidence_status::derived,
        1.0,
        "",
    });
    onset.attributes.push_back({
        "device_family",
        std::string{"YM2612"},
        evidence_status::derived,
        1.0,
        "",
    });
    onset.attributes.push_back({
        "device_pitch_code",
        fnum,
        evidence_status::exact,
        1.0,
        "fnum",
    });
    onset.attributes.push_back({
        "device_pitch_block",
        block,
        evidence_status::exact,
        1.0,
        "block",
    });
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realizes;
    realizes.kind = edge_kind::realizes;
    realizes.from = onset_id;
    realizes.to = episode_id;
    graph.add_edge(std::move(realizes));
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
                "genesis-role-test",
                "persistent part carries the principal motif",
                {part_id},
            },
            {
                part_role_evidence_kind::auditory_salience,
                part_role_evidence_origin::auditory_analysis,
                part_role_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.84,
                "genesis-role-test",
                "independent salience support",
                {part_id},
            },
        });
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id part_id = add_part(graph);
    add_episode(graph, part_id, 0, 80, 0x1234, 0x400, 4);
    add_episode(graph, part_id, 100, 180, 0x1234, 0x500, 4);

    const auto role = foreground_role(part_id);
    const auto summary = summarize_genesis_part_orchestration(graph, role);
    assert(summary.episode_ids.size() == 2);
    assert(summary.fm_program_fingerprints.size() == 1);
    assert(summary.all_relevant_fm_episodes_have_program_identity);
    assert(summary.register_center_log2_relative.has_value());

    const double expected_center =
        (std::log2(0x400 * 16.0) + std::log2(0x500 * 16.0)) / 2.0;
    assert(std::fabs(*summary.register_center_log2_relative - expected_center) < 1e-12);

    const auto state = make_genesis_part_orchestration_state(
        graph,
        role,
        "synthetic-genesis");
    assert(state.realization.has_value());
    assert(state.realization->basis == "ym2612_program_fingerprint");
    assert(state.realization->identity == "0000000000001234");
    assert(state.register_coordinate.has_value());
    assert(state.register_basis == "genesis_device_relative_log2_pitch");

    // Mixed FM programs inside one role span are preserved as mixed realization
    // evidence rather than collapsed to whichever patch appeared first.
    musical_execution_graph mixed_graph;
    const node_id mixed_part = add_part(mixed_graph);
    add_episode(mixed_graph, mixed_part, 0, 80, 0x1234, 0x400, 4);
    add_episode(mixed_graph, mixed_part, 100, 180, 0x5678, 0x500, 4);
    const auto mixed_role = foreground_role(mixed_part);
    const auto mixed_state = make_genesis_part_orchestration_state(
        mixed_graph,
        mixed_role,
        "synthetic-genesis");
    assert(!mixed_state.realization.has_value());
    assert(mixed_state.register_coordinate.has_value());

    return 0;
}
