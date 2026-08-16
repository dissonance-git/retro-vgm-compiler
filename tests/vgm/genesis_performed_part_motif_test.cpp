#include "../../components/vgm/enhancement/genesis_part_motif_adapter.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;
using namespace gameaudio::vgm;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

node_id add_motif_episode(
    musical_execution_graph& graph,
    std::int64_t tick,
    std::uint16_t fnum,
    std::uint8_t algorithm,
    std::array<std::uint8_t, 4> multiples,
    bool detune_operator_one = false) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "YM2612 physical voice episode";
    episode.active = time_span{at(tick), at(tick + 80)};
    episode.attributes.push_back({
        "device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    const node_id episode_id = graph.add_node(std::move(episode));

    node onset;
    onset.kind = node_kind::musical_event;
    onset.layer = semantic_layer::musical_performance;
    onset.flow = flow_kind::event;
    onset.label = "YM2612 pitched activity onset";
    onset.active = time_span{at(tick), std::nullopt};
    onset.attributes.push_back({
        "event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_pitch_code", static_cast<std::uint64_t>(fnum), evidence_status::derived, 1.0, "device_native"});
    onset.attributes.push_back({
        "device_pitch_block", std::uint64_t{4}, evidence_status::derived, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realization;
    realization.kind = edge_kind::realizes;
    realization.from = onset_id;
    realization.to = episode_id;
    graph.add_edge(std::move(realization));

    ym2612_episode_synthesis_snapshot snapshot;
    snapshot.instance = 0;
    snapshot.channel_index = 0;
    snapshot.channel.fnum = fnum;
    snapshot.channel.block = 4;
    snapshot.channel.algorithm = algorithm;
    snapshot.channel.feedback = 0;
    snapshot.channel.operator_key_mask = 0x0F;
    snapshot.channel.key_on = true;
    snapshot.channel.ams = 0;
    snapshot.channel.fms = 0;
    for (std::size_t index = 0; index < snapshot.channel.operators.size(); ++index) {
        auto& op = snapshot.channel.operators[index];
        op.multiple = multiples[index];
        op.detune = detune_operator_one && index == 0 ? 1 : 0;
        op.total_level = 0;
    }
    snapshot.lfo_enabled = false;
    snapshot.lfo_frequency = 0;

    add_ym2612_episode_synthesis_snapshot(
        graph,
        episode_id,
        snapshot,
        at(tick),
        "performed-motif-test",
        static_cast<std::uint64_t>(tick));
    return episode_id;
}

node_id add_part(
    musical_execution_graph& graph,
    const std::vector<node_id>& episodes,
    double confidence = 0.93) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = "persistent musical part";
    part.active = time_span{at(episodes.empty() ? 0 : 100), std::nullopt};
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    const node_id part_id = graph.add_node(std::move(part));

    for (node_id episode_id : episodes) {
        edge membership;
        membership.kind = edge_kind::groups_into;
        membership.from = episode_id;
        membership.to = part_id;
        graph.add_edge(std::move(membership));
    }
    return part_id;
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) <= tolerance;
}

struct motif_fixture {
    musical_execution_graph graph;
    node_id part_id = 0;
};

motif_fixture multiplier_fixture(bool detune_last = false) {
    motif_fixture result;
    const std::vector<node_id> episodes = {
        add_motif_episode(result.graph, 100, 1000, 7, {1, 1, 1, 1}),
        add_motif_episode(result.graph, 200, 1000, 7, {2, 2, 2, 2}),
        add_motif_episode(result.graph, 300, 1000, 7, {1, 1, 1, 1}, detune_last),
    };
    result.part_id = add_part(result.graph, episodes);
    return result;
}

motif_fixture missing_fundamental_fixture() {
    motif_fixture result;
    const std::vector<node_id> episodes = {
        add_motif_episode(result.graph, 100, 1000, 0, {1, 1, 1, 2}),
        add_motif_episode(result.graph, 200, 1200, 0, {1, 1, 1, 2}),
        add_motif_episode(result.graph, 300, 1000, 0, {1, 1, 1, 2}),
    };
    result.part_id = add_part(result.graph, episodes);
    return result;
}

motif_fixture direct_fixture() {
    motif_fixture result;
    const std::vector<node_id> episodes = {
        add_motif_episode(result.graph, 100, 1000, 7, {1, 1, 1, 1}),
        add_motif_episode(result.graph, 200, 1200, 7, {1, 1, 1, 1}),
        add_motif_episode(result.graph, 300, 1000, 7, {1, 1, 1, 1}),
    };
    result.part_id = add_part(result.graph, episodes);
    return result;
}

} // namespace

int main() {
    const genesis_pitch_clock_context clocks{
        7670454,
        0,
        "synthetic-vgm-header",
    };

    {
        auto fixture = multiplier_fixture();
        const auto native = make_genesis_part_motif_profile(
            fixture.graph,
            fixture.part_id);
        const auto performed = make_genesis_part_motif_profile(
            fixture.graph,
            fixture.part_id,
            clocks);
        assert(native.has_value());
        assert(performed.has_value());
        assert(native->pitch_basis == "genesis_ym2612_relative_frequency_code");
        assert(performed->pitch_basis == "absolute_performed_frequency_hz");
        assert(native->interval_octaves.has_value());
        assert(performed->interval_octaves.has_value());
        assert(native->interval_octaves->size() == 2);
        assert(performed->interval_octaves->size() == 2);

        // Identical channel FNUMs look stationary in the native coordinate,
        // while the operator network reveals 1x -> 2x -> 1x performed pitch.
        assert(close_enough((*native->interval_octaves)[0], 0.0));
        assert(close_enough((*native->interval_octaves)[1], 0.0));
        assert(close_enough((*performed->interval_octaves)[0], 1.0));
        assert(close_enough((*performed->interval_octaves)[1], -1.0));
        assert(close_enough(
            performed->evidence_confidence,
            ym2612_direct_periodicity_pitch_confidence));
        assert(performed->status == evidence_status::hypothesis);
    }

    {
        auto fixture = multiplier_fixture(true);
        const auto clock_aware = make_genesis_part_motif_profile(
            fixture.graph,
            fixture.part_id,
            clocks);
        assert(clock_aware.has_value());

        // One detuned episode cannot support the static operator-network pitch
        // interpretation. The entire part falls back to one coherent native
        // coordinate instead of mixing two semantic bases.
        assert(clock_aware->pitch_basis == "genesis_ym2612_relative_frequency_code");
        assert(clock_aware->interval_octaves.has_value());
        assert(close_enough((*clock_aware->interval_octaves)[0], 0.0));
        assert(close_enough((*clock_aware->interval_octaves)[1], 0.0));
        assert(close_enough(clock_aware->evidence_confidence, 1.0));
    }

    {
        auto missing = missing_fundamental_fixture();
        auto direct = direct_fixture();
        const auto missing_profile = make_genesis_part_motif_profile(
            missing.graph,
            missing.part_id,
            clocks);
        const auto direct_profile = make_genesis_part_motif_profile(
            direct.graph,
            direct.part_id,
            clocks);
        assert(missing_profile.has_value());
        assert(direct_profile.has_value());
        assert(missing_profile->pitch_basis == "absolute_performed_frequency_hz");
        assert(direct_profile->pitch_basis == "absolute_performed_frequency_hz");
        assert(close_enough(
            missing_profile->evidence_confidence,
            ym2612_missing_fundamental_pitch_ceiling));

        const auto comparison = compare_part_motif_profiles(
            *missing_profile,
            *direct_profile);
        assert(comparison.pitch_comparable);
        assert(close_enough(comparison.combined_similarity, 1.0));
        assert(close_enough(
            comparison.evidence_confidence,
            ym2612_missing_fundamental_pitch_ceiling));
        assert(close_enough(
            comparison.identity_confidence,
            ym2612_missing_fundamental_pitch_ceiling));
    }

    return 0;
}
