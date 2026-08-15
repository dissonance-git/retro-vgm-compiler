#include "../../model/realtime_musical_role_hypothesis.h"

#include <cassert>

namespace {

vgmtooling::model::realtime_source_spatial_observation make_observation(
    float low_band,
    float relative_energy,
    float activity,
    float edge,
    vgmtooling::model::spatial_audio_lane_kind kind =
        vgmtooling::model::spatial_audio_lane_kind::dry_source)
{
    vgmtooling::model::realtime_source_spatial_observation observation{};
    observation.audio_observed = true;
    observation.availability_fraction = 1.0f;
    observation.low_band_energy_ratio = low_band;
    observation.relative_energy = relative_energy;
    observation.activity = activity;
    observation.edge_ratio = edge;
    observation.lane_kind = kind;
    observation.source_age_seconds = 0.5f;
    return observation;
}

} // namespace

int main()
{
    using namespace vgmtooling::model;

    realtime_musical_role_proposer proposer{};
    spatial_source_evidence source{};
    realtime_scene_spatial_observation scene{};
    scene.audio_observed = true;
    scene.energy_concentration = 0.5f;

    // Strong low-band body can nominate a foundation role, but raw acoustics
    // alone are deliberately prevented from becoming a high-confidence bass
    // verdict.
    const auto low_source = proposer.propose(
        source,
        make_observation(0.95f, 0.80f, 0.90f, 0.05f),
        scene);
    assert(low_source.foundation.score > 0.50f);
    assert(low_source.foundation.confidence <= 0.300001f);
    assert(low_source.foreground.confidence <= 0.150001f);

    // Even an extremely salient source remains weak foreground evidence until
    // melodic/rhythmic/part context can distinguish lead from loud support.
    const auto loud_source = proposer.propose(
        source,
        make_observation(0.10f, 1.0f, 1.0f, 0.80f),
        scene);
    assert(loud_source.foreground.score > 0.80f);
    assert(loud_source.foreground.confidence <= 0.150001f);

    // A lane whose source contract explicitly identifies it as the shared wet
    // return may strongly support an environmental-layer interpretation while
    // still saying nothing about authored 3-D coordinates.
    const auto wet_source = proposer.propose(
        source,
        make_observation(
            0.20f,
            0.40f,
            0.70f,
            0.10f,
            spatial_audio_lane_kind::shared_effect_return),
        scene);
    assert(wet_source.environmental_layer.score > 0.90f);
    assert(wet_source.environmental_layer.confidence > 0.90f);

    // Stronger musical/presentation evidence is a prior. Conflicting weak raw
    // acoustics may reduce confidence slightly, but must not yank that prior
    // toward the weak cue as though both evidence layers were equal.
    source.presentation.foundation = 0.90f;
    source.presentation.foreground = 0.70f;
    source.presentation.diffuse = 0.20f;
    source.presentation.confidence = 0.80f;
    const auto prior = proposer.propose(
        source,
        make_observation(0.10f, 0.20f, 0.30f, 0.10f),
        scene);
    assert(prior.foundation.confidence < 0.80f);
    assert(prior.foundation.confidence > 0.70f);
    assert(prior.foundation.score > 0.75f);
    assert(prior.foreground.confidence > 0.75f);

    // No observed PCM means no acoustic invention. Existing higher evidence is
    // preserved, while an acoustic transient hypothesis remains unavailable.
    const realtime_source_spatial_observation no_audio{};
    const auto preserved = proposer.propose(source, no_audio, scene);
    assert(preserved.foundation.score == 0.90f);
    assert(preserved.foundation.confidence == 0.80f);
    assert(preserved.transient_accent.confidence == 0.0f);

    return 0;
}
