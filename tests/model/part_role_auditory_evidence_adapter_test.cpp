#include "model/part_role_auditory_evidence_adapter.h"
#include "model/realtime_relational_auditory_salience.h"
#include "model/spatial_source_part_binding.h"

#include <array>
#include <cassert>
#include <cmath>

using namespace vgmtooling::model;

namespace {

time_span role_window() {
    return {
        time_coordinate{time_domain::source, 0, 1000, 0},
        time_coordinate{time_domain::source, 2000, 1000, 0},
    };
}

bool contains_role(
    const part_role_window_result& result,
    musical_part_role role) {
    for (const auto& candidate : result.candidates) {
        if (candidate.role == role)
            return true;
    }
    return false;
}

realtime_source_spatial_observation salient_observation() {
    realtime_source_spatial_observation observation{};
    observation.audio_observed = true;
    observation.source_id = 9001;
    observation.generation = 3;
    observation.availability_fraction = 1.0f;
    observation.low_band_energy_ratio = 0.10f;
    observation.relative_energy = 1.0f;
    observation.activity = 1.0f;
    observation.edge_ratio = 0.80f;
    observation.lane_kind = spatial_audio_lane_kind::dry_source;
    observation.source_age_seconds = 0.5f;
    return observation;
}

std::array<realtime_source_spatial_observation, 2>
relational_scene(bool spectrally_distinct, float target_age_seconds = 0.5f) {
    std::array<realtime_source_spatial_observation, 2> observations{};

    auto& target = observations[0];
    target.audio_observed = true;
    target.source_id = 9001;
    target.generation = 3;
    target.lane_kind = spatial_audio_lane_kind::dry_source;
    target.availability_fraction = 1.0f;
    target.activity = 1.0f;
    target.relative_energy = 0.90f;
    target.edge_ratio = 0.60f;
    target.source_age_seconds = target_age_seconds;
    target.coarse_band_energy_share = {0.05f, 0.15f, 0.80f};

    auto& competitor = observations[1];
    competitor.audio_observed = true;
    competitor.source_id = 9002;
    competitor.generation = 1;
    competitor.lane_kind = spatial_audio_lane_kind::dry_source;
    competitor.availability_fraction = 1.0f;
    competitor.activity = 1.0f;
    competitor.relative_energy = 0.10f;
    competitor.edge_ratio = 0.20f;
    competitor.source_age_seconds = 0.5f;
    competitor.coarse_band_energy_share = spectrally_distinct
        ? std::array<float, 3>{0.80f, 0.15f, 0.05f}
        : target.coarse_band_energy_share;

    return observations;
}

} // namespace

int main() {
    node part{};
    part.id = 42;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        0.80,
        "",
    });

    spatial_source_evidence source{};
    source.source_id = 9001;
    source.generation = 3;
    source.family = spatial_source_family::vgm;
    source = bind_spatial_source_to_persistent_part(source, part);
    assert(source.source_id == 9001);
    assert(source.persistent_part_present);
    assert(source.persistent_part_id == part.id);
    assert(std::fabs(source.persistent_part_confidence - 0.80f) < 1.0e-6f);

    part_role_window_descriptor descriptor{};
    descriptor.part_id = part.id;
    descriptor.active = role_window();
    descriptor.onset_count = 8;
    descriptor.structural_motif_prominence = bounded_role_signal{1.0, 1.0};

    realtime_scene_spatial_observation scene{};
    scene.audio_observed = true;
    scene.energy_concentration = 0.5f;
    realtime_musical_role_proposer proposer{};

    // Current raw acoustics may be salient, but their foreground confidence is
    // deliberately too weak to satisfy the high-level role kernel. Binding the
    // evidence to a persistent part must preserve that cap rather than promote it.
    const auto raw_roles = proposer.propose(source, salient_observation(), scene);
    assert(raw_roles.foreground.score > 0.80f);
    assert(raw_roles.foreground.confidence <= 0.150001f);

    auto raw_descriptor = descriptor;
    assert(attach_realtime_auditory_salience(raw_descriptor, source, raw_roles));
    assert(raw_descriptor.auditory_salience.has_value());
    assert(std::fabs(
        raw_descriptor.auditory_salience->confidence -
        static_cast<double>(raw_roles.foreground.confidence)) < 1.0e-6);
    assert(!role_signal_strength_if_usable(raw_descriptor.auditory_salience).has_value());
    const auto raw_inference = infer_part_roles_for_window(
        {raw_descriptor},
        "auditory-part-binding-test");
    assert(!contains_role(raw_inference, musical_part_role::melodic_foreground));

    // Role-derived presentation memory cannot loop back through the realtime
    // proposer and masquerade as an independent auditory discriminator.
    auto feedback_source = source;
    feedback_source.presentation.foreground = 0.95f;
    feedback_source.presentation.confidence = 0.90f;
    const auto feedback_roles = proposer.propose(
        feedback_source,
        salient_observation(),
        scene);
    assert(realtime_role_hypothesis_uses_cue(
        feedback_roles.foreground,
        realtime_musical_role_cue::presentation_prior));
    auto feedback_descriptor = descriptor;
    assert(!attach_realtime_auditory_salience(
        feedback_descriptor,
        feedback_source,
        feedback_roles));
    assert(!feedback_descriptor.auditory_salience.has_value());

    // Stronger auditory confidence is earned by a different question, not by
    // raising the raw-acoustic cap: this stable source owns most scene energy
    // and is spectrally distinct from an active dry competitor. The analyzer
    // never reads the role-derived presentation prior.
    const auto distinct_scene = relational_scene(true);
    const auto relational = propose_relational_auditory_salience(
        source,
        distinct_scene.data(),
        distinct_scene.size());
    assert(relational.competitor_count == 1);
    assert(relational.spectral_distinctiveness > 0.80f);
    assert(relational.hypotheses.foreground.score > 0.85f);
    assert(std::fabs(relational.hypotheses.foreground.confidence - 0.70f) < 1.0e-6f);
    assert(realtime_role_hypothesis_uses_cue(
        relational.hypotheses.foreground,
        realtime_musical_role_cue::spectral_contrast));
    assert(realtime_role_hypothesis_uses_cue(
        relational.hypotheses.foreground,
        realtime_musical_role_cue::source_continuity));
    assert(!realtime_role_hypothesis_uses_cue(
        relational.hypotheses.foreground,
        realtime_musical_role_cue::presentation_prior));

    auto relational_descriptor = descriptor;
    assert(attach_realtime_auditory_salience(
        relational_descriptor,
        source,
        relational.hypotheses));
    assert(relational_descriptor.auditory_salience.has_value());
    assert(std::fabs(relational_descriptor.auditory_salience->confidence - 0.70) < 1.0e-6);
    assert(role_signal_strength_if_usable(relational_descriptor.auditory_salience).has_value());
    const auto relational_inference = infer_part_roles_for_window(
        {relational_descriptor},
        "relational-auditory-salience-test");
    assert(contains_role(relational_inference, musical_part_role::melodic_foreground));

    // Equal loudness is not enough. With the same broad spectral profile as the
    // active competitor, the target remains audible and energetic but its
    // relational salience stays below the role-use threshold.
    const auto blended_scene = relational_scene(false);
    const auto blended = propose_relational_auditory_salience(
        source,
        blended_scene.data(),
        blended_scene.size());
    assert(blended.competitor_count == 1);
    assert(blended.spectral_distinctiveness < 1.0e-6f);
    assert(blended.hypotheses.foreground.confidence > 0.0f);
    auto blended_descriptor = descriptor;
    assert(attach_realtime_auditory_salience(
        blended_descriptor,
        source,
        blended.hypotheses));
    assert(blended_descriptor.auditory_salience.has_value());
    assert(!role_signal_strength_if_usable(blended_descriptor.auditory_salience).has_value());
    const auto blended_inference = infer_part_roles_for_window(
        {blended_descriptor},
        "blended-auditory-control");
    assert(!contains_role(blended_inference, musical_part_role::melodic_foreground));

    // A newly appeared source has not earned enough temporal reliability yet,
    // even when its instantaneous spectral contrast is strong.
    const auto young_scene = relational_scene(true, 0.10f);
    const auto young = propose_relational_auditory_salience(
        source,
        young_scene.data(),
        young_scene.size());
    assert(young.hypotheses.foreground.score > 0.85f);
    assert(young.hypotheses.foreground.confidence < 0.20f);
    auto young_descriptor = descriptor;
    assert(attach_realtime_auditory_salience(
        young_descriptor,
        source,
        young.hypotheses));
    assert(!role_signal_strength_if_usable(young_descriptor.auditory_salience).has_value());

    // With no active dry competitor, there is no relational contrast claim to
    // make. The analyzer must not smuggle the old single-source loudness cue in
    // under the stronger confidence ceiling.
    const std::array<realtime_source_spatial_observation, 1> solo{
        distinct_scene[0],
    };
    const auto solo_result = propose_relational_auditory_salience(
        source,
        solo.data(),
        solo.size());
    assert(solo_result.competitor_count == 0);
    assert(solo_result.hypotheses.foreground.confidence == 0.0f);

    // Missing or contradictory persistent-part identity stays missing rather
    // than being joined by source id, physical slot, or coincidental equality.
    spatial_source_evidence unbound{};
    unbound.source_id = part.id;
    auto unbound_descriptor = descriptor;
    assert(!attach_realtime_auditory_salience(
        unbound_descriptor,
        unbound,
        relational.hypotheses));

    auto wrong_descriptor = descriptor;
    wrong_descriptor.part_id = part.id + 1;
    assert(!attach_realtime_auditory_salience(
        wrong_descriptor,
        source,
        relational.hypotheses));

    return 0;
}
