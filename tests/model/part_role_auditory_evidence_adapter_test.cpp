#include "model/part_role_auditory_evidence_adapter.h"
#include "model/spatial_source_part_binding.h"

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
    observation.availability_fraction = 1.0f;
    observation.low_band_energy_ratio = 0.10f;
    observation.relative_energy = 1.0f;
    observation.activity = 1.0f;
    observation.edge_ratio = 0.80f;
    observation.lane_kind = spatial_audio_lane_kind::dry_source;
    observation.source_age_seconds = 0.5f;
    return observation;
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

    // A genuinely stronger independent auditory analyzer can use the same
    // bridge in the future. Its confidence is still capped by persistent-part
    // identity confidence before entering role inference.
    realtime_musical_role_hypotheses independent_roles{};
    independent_roles.foreground = {
        0.90f,
        0.85f,
        role_cue_mask(realtime_musical_role_cue::relative_energy)
            | role_cue_mask(realtime_musical_role_cue::activity),
    };
    auto independent_descriptor = descriptor;
    assert(attach_realtime_auditory_salience(
        independent_descriptor,
        source,
        independent_roles));
    assert(independent_descriptor.auditory_salience.has_value());
    assert(std::fabs(independent_descriptor.auditory_salience->value - 0.90) < 1.0e-6);
    assert(std::fabs(independent_descriptor.auditory_salience->confidence - 0.80) < 1.0e-6);
    const auto independent_inference = infer_part_roles_for_window(
        {independent_descriptor},
        "auditory-part-binding-test");
    assert(contains_role(independent_inference, musical_part_role::melodic_foreground));

    // Missing or contradictory identity stays missing rather than being joined
    // by source id, physical slot, or coincidental numeric equality.
    spatial_source_evidence unbound{};
    unbound.source_id = part.id;
    auto unbound_descriptor = descriptor;
    assert(!attach_realtime_auditory_salience(
        unbound_descriptor,
        unbound,
        independent_roles));

    auto wrong_descriptor = descriptor;
    wrong_descriptor.part_id = part.id + 1;
    assert(!attach_realtime_auditory_salience(
        wrong_descriptor,
        source,
        independent_roles));

    return 0;
}
