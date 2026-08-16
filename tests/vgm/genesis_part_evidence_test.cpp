#include "components/vgm/enhancement/genesis_part_evidence.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

namespace {

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    std::uint64_t channel,
    std::uint64_t fingerprint) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = "YM2612 physical voice episode";
    value.active = time_span{
        {time_domain::source, start, 0, 0},
        time_coordinate{time_domain::source, end, 0, 0},
    };
    value.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"physical_channel", channel, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"instrument_program_fingerprint", fingerprint, evidence_status::derived, 1.0, "fnv1a64"});
    return graph.add_node(std::move(value));
}

node_id add_onset(
    musical_execution_graph& graph,
    node_id episode,
    std::int64_t tick,
    std::uint16_t fnum,
    std::uint8_t block) {
    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = "YM2612 pitched_activity_onset";
    event.active = time_span{{time_domain::source, tick, 0, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_pitch_code", static_cast<std::uint64_t>(fnum), evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_pitch_block", static_cast<std::uint64_t>(block), evidence_status::derived, 1.0, ""});
    const node_id event_id = graph.add_node(std::move(event));

    edge relation;
    relation.kind = edge_kind::realizes;
    relation.from = event_id;
    relation.to = episode;
    graph.add_edge(std::move(relation));
    return event_id;
}

bool has_evidence(
    const persistent_part_hypothesis& hypothesis,
    persistent_part_evidence_kind kind,
    persistent_part_evidence_polarity polarity) {
    for (const auto& item : hypothesis.evidence) {
        if (item.kind == kind && item.polarity == polarity)
            return true;
    }
    return false;
}

} // namespace

int main() {
    ym2612_channel_state patch_a{};
    patch_a.algorithm = 4;
    patch_a.feedback = 6;
    patch_a.ams = 1;
    patch_a.fms = 3;
    patch_a.fnum = 0x234;
    patch_a.block = 3;
    patch_a.pan_left = true;
    patch_a.pan_right = false;
    patch_a.operators[0].multiple = 2;
    patch_a.operators[0].total_level = 18;
    patch_a.operators[1].detune = 3;
    patch_a.operators[2].attack_rate = 29;
    patch_a.operators[3].ssg_eg = 5;

    ym2612_channel_state same_program = patch_a;
    same_program.fnum = 0x512;
    same_program.block = 5;
    same_program.pan_left = false;
    same_program.pan_right = true;
    same_program.operator_key_mask = 0x0f;

    const std::uint64_t fingerprint = ym2612_program_fingerprint(patch_a);
    assert(fingerprint == ym2612_program_fingerprint(same_program));

    ym2612_channel_state changed_program = same_program;
    changed_program.operators[0].multiple = 7;
    assert(fingerprint != ym2612_program_fingerprint(changed_program));

    musical_execution_graph graph;
    const node_id first = add_episode(graph, 0, 1000, 0, fingerprint);
    const node_id second = add_episode(graph, 1100, 2000, 0, fingerprint);
    const node_id overlapping = add_episode(graph, 800, 1600, 1, fingerprint);
    add_onset(graph, first, 0, 0x300, 3);
    add_onset(graph, second, 1100, 0x360, 3);
    add_onset(graph, overlapping, 800, 0x320, 3);

    genesis_part_continuity_policy policy;
    policy.max_gap_ticks = 500;
    policy.max_pitch_interval_octaves = 1.5;

    const auto strong = infer_genesis_persistent_part(
        graph,
        first,
        second,
        "synthetic-vgm",
        policy);
    assert(strong.identity_bearing_support);
    assert(strong.cross_domain_grounded);
    assert(strong.confidence >= 0.75);
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::physical_slot_continuity,
        persistent_part_evidence_polarity::supports));
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::instrument_program_identity,
        persistent_part_evidence_polarity::supports));
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::temporal_adjacency,
        persistent_part_evidence_polarity::supports));
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::pitch_trajectory_continuity,
        persistent_part_evidence_polarity::supports));

    const node_id part = add_persistent_part_hypothesis(graph, strong);
    assert(graph.find_node(part) != nullptr);
    assert(graph.edges_to(part, edge_kind::groups_into).size() == 2);

    const auto conflict = infer_genesis_persistent_part(
        graph,
        first,
        overlapping,
        "synthetic-vgm",
        policy);
    assert(conflict.strong_conflict_present);
    assert(conflict.confidence <= persistent_part_strong_conflict_confidence_ceiling);
    assert(has_evidence(
        conflict,
        persistent_part_evidence_kind::simultaneous_conflict,
        persistent_part_evidence_polarity::counters));

    // SN76489 tone-generator class alone is not a pitch-invariant instrument
    // program identity and therefore should not receive a fake fingerprint.
    genesis_state state;
    musical_execution_graph annotation_graph;
    node psg;
    psg.kind = node_kind::voice_instance;
    psg.layer = semantic_layer::synthesis;
    psg.attributes.push_back({"device_family", std::string{"SN76489"}, evidence_status::derived, 1.0, ""});
    psg.attributes.push_back({"instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    psg.attributes.push_back({"physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    const node_id psg_id = annotation_graph.add_node(std::move(psg));
    annotate_genesis_episode_program_identity(annotation_graph, psg_id, state);
    const node* psg_node = annotation_graph.find_node(psg_id);
    assert(psg_node != nullptr);
    assert(find_genesis_part_attribute(*psg_node, "instrument_program_fingerprint") == nullptr);
    assert(find_genesis_part_attribute(*psg_node, "instrument_program_scope") != nullptr);

    return 0;
}
