#include "components/spc/spc_absolute_pitch_adapter.h"
#include "model/harmonic_verticality_timeline.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::device, tick, 32000, 0};
}

node_id add_sample(musical_execution_graph& graph, const char* label) {
    node sample;
    sample.kind = node_kind::sample_buffer;
    sample.layer = semantic_layer::synthesis;
    sample.flow = flow_kind::value;
    sample.label = label;
    sample.attributes.push_back({
        "encoding",
        std::string{"BRR"},
        evidence_status::exact,
        1.0,
        "",
    });
    return graph.add_node(std::move(sample));
}

node_id add_episode(
    musical_execution_graph& graph,
    std::uint64_t voice,
    std::int64_t start,
    std::int64_t end) {
    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "S-DSP physical voice episode";
    episode.active = time_span{at(start), at(end)};
    episode.attributes.push_back({
        "physical_voice",
        voice,
        evidence_status::exact,
        1.0,
        "slot",
    });
    return graph.add_node(std::move(episode));
}

node_id add_pitch_event(
    musical_execution_graph& graph,
    node_id episode,
    node_id sample,
    std::uint64_t voice,
    std::int64_t tick,
    std::uint64_t pitch_rate,
    bool noise_enabled = false) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = "S-DSP key_on_accepted";
    event.active = time_span{at(tick), std::nullopt};
    event.attributes.push_back({
        "event_kind",
        std::string{"key_on_accepted"},
        evidence_status::exact,
        1.0,
        "",
    });
    event.attributes.push_back({
        "physical_voice",
        voice,
        evidence_status::exact,
        1.0,
        "slot",
    });
    event.attributes.push_back({
        "source_index",
        voice,
        evidence_status::exact,
        1.0,
        "slot",
    });
    event.attributes.push_back({
        "pitch_rate",
        pitch_rate,
        evidence_status::exact,
        1.0,
        "device_native",
    });
    event.attributes.push_back({
        "noise_enabled",
        noise_enabled,
        evidence_status::exact,
        1.0,
        "",
    });
    const node_id event_id = graph.add_node(std::move(event));

    edge causes;
    causes.kind = edge_kind::causes;
    causes.from = event_id;
    causes.to = episode;
    graph.add_edge(std::move(causes));

    edge references;
    references.kind = edge_kind::references;
    references.from = event_id;
    references.to = sample;
    graph.add_edge(std::move(references));
    return event_id;
}

node_id add_part(
    musical_execution_graph& graph,
    node_id episode,
    double confidence = 0.92) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = "persistent musical part";
    part.active = graph.find_node(episode)->active;
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    const node_id part_id = graph.add_node(std::move(part));

    edge membership;
    membership.kind = edge_kind::groups_into;
    membership.from = episode;
    membership.to = part_id;
    graph.add_edge(std::move(membership));
    return part_id;
}

const absolute_musical_pitch_observation* find_part(
    const std::vector<absolute_musical_pitch_observation>& observations,
    node_id part_id) {
    for (const auto& observation : observations) {
        if (observation.part_id == part_id)
            return &observation;
    }
    return nullptr;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id sample_a = add_sample(graph, "BRR sample A");
    const node_id sample_b = add_sample(graph, "BRR sample B");
    const node_id sample_c = add_sample(graph, "BRR unused sample C");

    const node_id episode_a = add_episode(graph, 0, 0, 3200);
    const node_id episode_b = add_episode(graph, 1, 0, 3200);
    add_pitch_event(graph, episode_a, sample_a, 0, 0, 0x1800);
    add_pitch_event(graph, episode_b, sample_b, 1, 0, 0x1000);
    const node_id part_a = add_part(graph, episode_a, 0.92);
    const node_id part_b = add_part(graph, episode_b, 0.90);

    const auto authored_a = make_sample_root_tuning_evidence(
        sample_a,
        sample_root_tuning_origin::authored_metadata,
        sample_root_tuning_state::pitched,
        220.0,
        evidence_status::exact,
        1.0,
        "authored-instrument-tuning");
    const auto auditory_b = make_sample_root_tuning_evidence(
        sample_b,
        sample_root_tuning_origin::auditory_fundamental_estimate,
        sample_root_tuning_state::pitched,
        440.0,
        evidence_status::hypothesis,
        0.99,
        "auditory-f0-estimate");

    // Auditory estimation remains explicitly bounded even when the proposing
    // analyzer reports a larger confidence. Exact authored tuning may retain 1.
    assert(std::fabs(authored_a.confidence - 1.0) < 1.0e-12);
    assert(std::fabs(auditory_b.confidence - 0.80) < 1.0e-12);

    const auto observations = collect_spc_absolute_performed_pitch_observations(
        graph,
        {authored_a, auditory_b},
        "spc-absolute-pitch-test");
    assert(observations.size() == 2);

    const auto* a = find_part(observations, part_a);
    const auto* b = find_part(observations, part_b);
    assert(a != nullptr && b != nullptr);
    // A's exact S-DSP pitch rate transposes its 220 Hz unity sample by 1.5x.
    assert(std::fabs(a->frequency_hz - 330.0) < 1.0e-9);
    assert(std::fabs(b->frequency_hz - 440.0) < 1.0e-9);
    assert(std::fabs(a->confidence - 0.92) < 1.0e-12);
    assert(std::fabs(b->confidence - 0.80) < 1.0e-12);
    assert(a->role == musical_pitch_role::performed);
    assert(b->role == musical_pitch_role::performed);

    // Different samples become harmonically comparable only after each exact
    // sample version has separately earned a unity-playback tuning.
    const auto verticalities = make_harmonic_verticality_timeline(observations);
    assert(verticalities.size() == 1);
    assert(verticalities.front().observation_time.tick == 0);
    assert(verticalities.front().part_ids.size() == 2);
    assert(std::fabs(verticalities.front().confidence - 0.80) < 1.0e-12);

    // Ambiguous or unpitched tuning is explicit absence of absolute-pitch
    // authority, not permission to use pitch_rate as if it were Hz.
    const auto ambiguous_b = make_sample_root_tuning_evidence(
        sample_b,
        sample_root_tuning_origin::auditory_fundamental_estimate,
        sample_root_tuning_state::ambiguous,
        0.0,
        evidence_status::hypothesis,
        0.75,
        "octave-ambiguous-f0");
    const auto ambiguous_observations = collect_spc_absolute_performed_pitch_observations(
        graph,
        {authored_a, ambiguous_b},
        "spc-absolute-pitch-test");
    assert(ambiguous_observations.size() == 1);
    assert(make_harmonic_verticality_timeline(ambiguous_observations).empty());

    const auto unpitched_b = make_sample_root_tuning_evidence(
        sample_b,
        sample_root_tuning_origin::auditory_fundamental_estimate,
        sample_root_tuning_state::unpitched,
        0.0,
        evidence_status::hypothesis,
        0.90,
        "percussive-sample-control");
    assert(collect_spc_absolute_performed_pitch_observations(
        graph,
        {authored_a, unpitched_b},
        "spc-absolute-pitch-test").size() == 1);

    // A tuning claim for another exact BRR version cannot calibrate this event.
    const auto wrong_sample = make_sample_root_tuning_evidence(
        sample_c,
        sample_root_tuning_origin::authored_metadata,
        sample_root_tuning_state::pitched,
        440.0,
        evidence_status::exact,
        1.0,
        "different-sample-tuning");
    assert(spc_episode_absolute_pitch_observations(
        graph,
        episode_b,
        wrong_sample,
        "spc-absolute-pitch-test").empty());

    // Competing tuning claims for one exact sample version remain ambiguous at
    // this layer instead of being averaged into invented certainty.
    bool competing_rejected = false;
    try {
        auto second_b = auditory_b;
        second_b.unity_playback_fundamental_hz = 220.0;
        (void)collect_spc_absolute_performed_pitch_observations(
            graph,
            {auditory_b, second_b},
            "spc-absolute-pitch-test");
    } catch (const std::invalid_argument&) {
        competing_rejected = true;
    }
    assert(competing_rejected);

    // S-DSP noise mode is not a pitched sample observation even when a tuning
    // exists for the referenced sample object.
    const node_id noise_episode = add_episode(graph, 2, 4000, 5000);
    add_pitch_event(graph, noise_episode, sample_a, 2, 4000, 0x1000, true);
    add_part(graph, noise_episode, 0.95);
    assert(spc_episode_absolute_pitch_observations(
        graph,
        noise_episode,
        authored_a,
        "spc-absolute-pitch-test").empty());

    // Exact-upstream calibration is stronger than generic auditory estimation,
    // but still bounded below authored metadata because pitch calibration itself
    // remains an interpreted property of the waveform.
    const auto upstream = make_sample_root_tuning_evidence(
        sample_c,
        sample_root_tuning_origin::exact_upstream_calibration,
        sample_root_tuning_state::pitched,
        261.625565,
        evidence_status::derived,
        1.0,
        "exact-upstream-pcm-calibration");
    assert(std::fabs(upstream.confidence - 0.95) < 1.0e-12);

    return 0;
}
