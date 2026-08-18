#pragma once

#include "realtime_musical_spatial_observer.h"

#include <algorithm>
#include <cmath>

namespace vgmtooling::model {

// A content-adaptive *presentation* budget, not a musical classifier.
//
// The target aesthetic stays constant: make source-native music substantially
// larger without losing hierarchy, impact, or identity. The budget only decides
// where that spatial scale is safest to spend for the soundtrack currently being
// observed. Values are dimensionless multipliers around a neutral value of 1.
struct realtime_spatial_mix_budget {
    float dry_width_scale = 1.0f;
    float dry_diffuse_scale = 1.0f;
    float depth_scale = 1.0f;
    float height_scale = 1.0f;
    float shared_wet_strength = 1.0f;
    float shared_wet_extent = 1.0f;
    float added_externalization_scale = 1.0f;
};

struct realtime_spatial_mix_budget_policy {
    // If the scene suddenly becomes dense/wet/transient-heavy, pull the added
    // spatial treatment back quickly. Open the scene more slowly so a quiet or
    // sparse passage does not cause the image to pump outward on one callback.
    float contraction_seconds = 0.30f;
    float expansion_seconds = 1.50f;
};

inline float clamp_mix_budget(float value, float low, float high) noexcept {
    if (!std::isfinite(value))
        return 1.0f;
    return std::clamp(value, low, high);
}

// Convert source-agnostic scene measurements into a bounded target. No genre,
// game, composer, or soundtrack identity participates. Two recordings with the
// same causal mix geometry therefore receive the same intervention budget.
inline realtime_spatial_mix_budget target_realtime_spatial_mix_budget(
    const realtime_scene_spatial_observation& scene) noexcept
{
    realtime_spatial_mix_budget out{};
    if (!scene.audio_observed || scene.observed_lane_count == 0)
        return out;

    const float observed = static_cast<float>(scene.observed_lane_count);
    const float density = std::clamp(
        static_cast<float>(scene.active_lane_count) / observed,
        0.0f,
        1.0f);
    const float open_space = 1.0f - density;
    const float concentration = std::clamp(scene.energy_concentration, 0.0f, 1.0f);
    const float distributed = 1.0f - concentration;
    const float wet = std::clamp(scene.shared_effect_energy_share, 0.0f, 1.0f);
    const float low = std::clamp(scene.low_band_energy_ratio, 0.0f, 1.0f);
    const float edge = std::clamp(scene.edge_ratio, 0.0f, 1.0f);
    const float overlap = std::clamp(scene.coarse_spectral_overlap, 0.0f, 1.0f);

    // Pairwise spectral overlap only becomes a useful crowding pressure when
    // there are active sources sharing the scene. Distributed energy makes that
    // competition more important than a scene where one source dominates. This
    // is explicitly a broad-band overlap proxy, not a psychoacoustic masking
    // probability.
    const float spectral_crowding = std::clamp(
        overlap * density * (0.60f + 0.40f * distributed),
        0.0f,
        1.0f);

    // Sparse/focused scenes can afford larger individual objects. Dense,
    // transient-rich, bass-heavy, or spectrally crowded scenes stay tighter to
    // protect articulation and the foundation. Spectral crowding reduces object
    // *extent* and diffuseness here; it does not pretend that making an object
    // wider is the same thing as spatially separating two masking sources.
    out.dry_width_scale = clamp_mix_budget(
        0.78f + 0.30f * open_space + 0.10f * concentration
            - 0.12f * edge - 0.08f * low - 0.16f * spectral_crowding,
        0.58f,
        1.15f);
    out.dry_diffuse_scale = clamp_mix_budget(
        0.68f + 0.24f * open_space + 0.10f * distributed
            - 0.20f * edge - 0.12f * low - 0.24f * spectral_crowding,
        0.38f,
        1.05f);

    // Depth and height are global capacity suggestions for the renderer. They
    // are intentionally conservative in low-heavy or very dense material and
    // become more available when the arrangement leaves perceptual room. True
    // masking-aware object separation will need its own explicit control; do not
    // smuggle that future behavior into these global capacity fields.
    out.depth_scale = clamp_mix_budget(
        0.82f + 0.20f * open_space + 0.12f * distributed - 0.10f * low,
        0.70f,
        1.12f);
    out.height_scale = clamp_mix_budget(
        0.78f + 0.22f * open_space + 0.10f * distributed
            - 0.10f * low - 0.08f * edge,
        0.65f,
        1.10f);

    // A source-native shared effect field is useful envelopment already. Let it
    // remain a strong spatial layer, especially when it is an important part of
    // the soundtrack, but trim some field size when dry sources are already
    // fighting for the same broad spectral territory.
    out.shared_wet_strength = clamp_mix_budget(
        0.72f + 0.18f * wet + 0.10f * distributed
            - 0.06f * spectral_crowding,
        0.60f,
        1.0f);
    out.shared_wet_extent = clamp_mix_budget(
        0.78f + 0.12f * wet + 0.10f * distributed
            - 0.08f * spectral_crowding,
        0.66f,
        1.0f);

    // Historical/shared wet and modern listening-room support compete for the
    // same perceptual real estate. An echo-heavy or spectrally crowded
    // soundtrack therefore asks for less added externalization than a dry,
    // clearly separated one.
    out.added_externalization_scale = clamp_mix_budget(
        0.98f - 0.72f * wet - 0.12f * density - 0.18f * spectral_crowding,
        0.16f,
        1.0f);
    return out;
}

class realtime_spatial_mix_budget_tracker {
public:
    explicit realtime_spatial_mix_budget_tracker(
        realtime_spatial_mix_budget_policy policy = {}) noexcept
        : policy_(policy) {}

    void reset() noexcept {
        current_ = {};
    }

    const realtime_spatial_mix_budget& current() const noexcept {
        return current_;
    }

    const realtime_spatial_mix_budget_policy& policy() const noexcept {
        return policy_;
    }

    bool set_policy(realtime_spatial_mix_budget_policy policy) noexcept {
        if (!valid_policy(policy))
            return false;
        policy_ = policy;
        return true;
    }

    bool observe(
        const realtime_scene_spatial_observation& scene,
        double block_seconds) noexcept
    {
        if (!std::isfinite(block_seconds) || block_seconds <= 0.0 || !valid_policy(policy_))
            return false;
        if (!scene.audio_observed || scene.observed_lane_count == 0)
            return true;

        const realtime_spatial_mix_budget target = target_realtime_spatial_mix_budget(scene);
        current_.dry_width_scale = smooth(current_.dry_width_scale, target.dry_width_scale, block_seconds);
        current_.dry_diffuse_scale = smooth(current_.dry_diffuse_scale, target.dry_diffuse_scale, block_seconds);
        current_.depth_scale = smooth(current_.depth_scale, target.depth_scale, block_seconds);
        current_.height_scale = smooth(current_.height_scale, target.height_scale, block_seconds);
        current_.shared_wet_strength = smooth(
            current_.shared_wet_strength,
            target.shared_wet_strength,
            block_seconds);
        current_.shared_wet_extent = smooth(
            current_.shared_wet_extent,
            target.shared_wet_extent,
            block_seconds);
        current_.added_externalization_scale = smooth(
            current_.added_externalization_scale,
            target.added_externalization_scale,
            block_seconds);
        return true;
    }

private:
    static bool valid_policy(const realtime_spatial_mix_budget_policy& policy) noexcept {
        return std::isfinite(policy.contraction_seconds) && policy.contraction_seconds > 0.0f
            && std::isfinite(policy.expansion_seconds) && policy.expansion_seconds > 0.0f;
    }

    float smooth(float current, float target, double block_seconds) const noexcept {
        const float seconds = target < current
            ? policy_.contraction_seconds
            : policy_.expansion_seconds;
        const double alpha = 1.0 - std::exp(-block_seconds / static_cast<double>(seconds));
        return static_cast<float>(
            static_cast<double>(current)
                + (static_cast<double>(target) - static_cast<double>(current)) * alpha);
    }

    realtime_spatial_mix_budget_policy policy_{};
    realtime_spatial_mix_budget current_{};
};

// ABI 0.4 gives scene-wide depth, height, historical-wet, and added-room
// controls their own renderer-side channel. Do not encode those decisions back
// into source evidence. Only dry-object width/diffuseness still use the existing
// per-source presentation vocabulary because they genuinely describe how that
// individual recovered object should occupy the remix.
inline spatial_source_evidence apply_realtime_spatial_mix_budget(
    spatial_audio_lane_kind lane_kind,
    const spatial_source_evidence& input,
    const realtime_spatial_mix_budget& budget) noexcept
{
    spatial_source_evidence out = input;
    if (lane_kind == spatial_audio_lane_kind::dry_source) {
        out.presentation.diffuse = clamp_unit_interval(
            out.presentation.diffuse * budget.dry_diffuse_scale);
        out.presentation.width = clamp_unit_interval(
            out.presentation.width * budget.dry_width_scale);
    }
    return out;
}

} // namespace vgmtooling::model