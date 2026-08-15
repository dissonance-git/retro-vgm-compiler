#pragma once

#include "realtime_musical_spatial_observer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace vgmtooling::model {

// Provenance bits for one bounded musical-role hypothesis. A cue bit means the
// cue actually participated in the score/confidence calculation; merely being
// available elsewhere in the source model does not earn provenance here.
enum class realtime_musical_role_cue : std::uint32_t {
    none = 0,
    presentation_prior = 1u << 0,
    low_band_body = 1u << 1,
    relative_energy = 1u << 2,
    activity = 1u << 3,
    edge_density = 1u << 4,
    source_continuity = 1u << 5,
    shared_effect_identity = 1u << 6,
    persistent_part_identity = 1u << 7,
};

constexpr std::uint32_t role_cue_mask(realtime_musical_role_cue cue) noexcept {
    return static_cast<std::uint32_t>(cue);
}

struct realtime_role_hypothesis {
    // score answers "how much does the current evidence resemble this role?"
    // confidence answers "how much right do we currently have to trust that?"
    // Keeping them separate prevents a strong-looking weak heuristic from
    // masquerading as settled musical understanding.
    float score = 0.0f;
    float confidence = 0.0f;
    std::uint32_t cues = 0;
};

struct realtime_musical_role_hypotheses {
    realtime_role_hypothesis foundation{};
    realtime_role_hypothesis foreground{};
    realtime_role_hypothesis transient_accent{};
    realtime_role_hypothesis environmental_layer{};
};

struct realtime_musical_role_policy {
    // Raw acoustics are intentionally weak evidence for musical function.
    // Stronger pitch/rhythm/part/form analyzers can later exceed these caps.
    float acoustic_foundation_confidence_cap = 0.30f;
    float acoustic_foreground_confidence_cap = 0.15f;
    float transient_confidence_cap = 0.30f;

    // A lane explicitly identified by the source bus as a shared effect return
    // is strong evidence of an environmental layer, though still not an
    // authored 3-D location.
    float shared_effect_confidence = 0.95f;
};

class realtime_musical_role_proposer {
public:
    explicit realtime_musical_role_proposer(
        realtime_musical_role_policy policy = {}) noexcept
        : policy_(policy) {}

    const realtime_musical_role_policy& policy() const noexcept {
        return policy_;
    }

    realtime_musical_role_hypotheses propose(
        const spatial_source_evidence& source,
        const realtime_source_spatial_observation& observation,
        const realtime_scene_spatial_observation& scene) const noexcept
    {
        realtime_musical_role_hypotheses out{};

        // Higher musical/perceptual evidence already attached to the source is
        // admitted as a prior, not flattened back into the acoustic baseline.
        const float prior_confidence = clamp_unit_interval(source.presentation.confidence);
        if (prior_confidence > 0.0f) {
            out.foundation = {
                clamp_unit_interval(source.presentation.foundation),
                prior_confidence,
                role_cue_mask(realtime_musical_role_cue::presentation_prior),
            };
            out.foreground = {
                clamp_unit_interval(source.presentation.foreground),
                prior_confidence,
                role_cue_mask(realtime_musical_role_cue::presentation_prior),
            };
            out.environmental_layer = {
                clamp_unit_interval(source.presentation.diffuse),
                prior_confidence,
                role_cue_mask(realtime_musical_role_cue::presentation_prior),
            };
        }

        if (!observation.audio_observed)
            return out;

        const float availability = clamp_unit_interval(observation.availability_fraction);
        const float activity = clamp_unit_interval(observation.activity);
        const float relative_energy = clamp_unit_interval(observation.relative_energy);
        const float low_band_body = clamp_unit_interval(observation.low_band_energy_ratio);
        const float edge_density = clamp_unit_interval(observation.edge_ratio);
        const float distributed_scene = scene.audio_observed
            ? clamp_unit_interval(1.0f - scene.energy_concentration)
            : 0.0f;

        // Low-frequency body + sustained audible energy is enough to nominate a
        // foundation candidate, but not enough to call it a bass part. Keep the
        // confidence deliberately low until pitch/rhythm/part evidence arrives.
        const float acoustic_foundation = low_band_body * activity
            * (0.5f + 0.5f * relative_energy);
        fuse(
            out.foundation,
            acoustic_foundation,
            policy_.acoustic_foundation_confidence_cap * availability,
            role_cue_mask(realtime_musical_role_cue::low_band_body)
                | role_cue_mask(realtime_musical_role_cue::relative_energy)
                | role_cue_mask(realtime_musical_role_cue::activity));

        // Acoustic salience alone is an especially weak foreground cue. A loud
        // accompaniment must not automatically jump in front of the melody.
        const float acoustic_foreground = relative_energy * activity
            * (0.75f + 0.25f * edge_density);
        fuse(
            out.foreground,
            acoustic_foreground,
            policy_.acoustic_foreground_confidence_cap * availability,
            role_cue_mask(realtime_musical_role_cue::relative_energy)
                | role_cue_mask(realtime_musical_role_cue::activity)
                | role_cue_mask(realtime_musical_role_cue::edge_density));

        // This is only a transient-accent candidate. Rhythmic function will be
        // decided later using onset timing plus phrase/metrical context.
        const float transient = edge_density * activity
            * (0.75f + 0.25f * distributed_scene);
        out.transient_accent = {
            transient,
            clamp_unit_interval(policy_.transient_confidence_cap * availability),
            role_cue_mask(realtime_musical_role_cue::edge_density)
                | role_cue_mask(realtime_musical_role_cue::activity),
        };

        if (observation.lane_kind == spatial_audio_lane_kind::shared_effect_return) {
            fuse(
                out.environmental_layer,
                1.0f,
                policy_.shared_effect_confidence * availability,
                role_cue_mask(realtime_musical_role_cue::shared_effect_identity));
        }

        return out;
    }

private:
    static void fuse(
        realtime_role_hypothesis& current,
        float score,
        float confidence,
        std::uint32_t cues) noexcept
    {
        score = clamp_unit_interval(score);
        confidence = clamp_unit_interval(confidence);
        if (!(confidence > 0.0f))
            return;

        if (!(current.confidence > 0.0f)) {
            current = {score, confidence, cues};
            return;
        }

        // Squared confidence weights keep a crude acoustic cue from dragging a
        // much stronger musical prior around the scene. The values are evidence
        // weights, not Bayesian probabilities.
        const float current_weight = current.confidence * current.confidence;
        const float incoming_weight = confidence * confidence;
        const float total_weight = current_weight + incoming_weight;
        const float disagreement = std::fabs(current.score - score);
        current.score = clamp_unit_interval(
            (current.score * current_weight + score * incoming_weight) / total_weight);

        // Agreement can strengthen a hypothesis slightly. Conflicting weak cues
        // instead reduce certainty rather than quietly increasing confidence.
        const float smaller_confidence = std::min(current.confidence, confidence);
        const float agreement = clamp_unit_interval(1.0f - disagreement);
        const float signed_support = (2.0f * agreement) - 1.0f;
        current.confidence = clamp_unit_interval(
            std::max(current.confidence, confidence)
                + 0.25f * smaller_confidence * signed_support);
        current.cues |= cues;
    }

    realtime_musical_role_policy policy_{};
};

} // namespace vgmtooling::model
