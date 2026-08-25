#pragma once

#include "realtime_musical_role_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace vgmtooling::model {

// This analyzer does not ask whether a source "is the melody." It asks a
// narrower perceptual question: does one stable dry causal source stand out
// against other simultaneously active dry sources through a conjunction of
// energy ownership, activity, edge structure, and coarse spectral separation?
// That relation is independent of motif/phrase analysis and can therefore
// corroborate it later, but it remains only auditory salience.
struct realtime_relational_auditory_salience_policy {
    float active_threshold = 0.15f;
    float full_continuity_age_seconds = 0.50f;

    // Multi-source contrast is stronger than the raw one-source acoustic
    // proposer, but it is still not proof of musical function. 0.70 is an
    // epistemic ceiling, not a calibrated probability.
    float confidence_cap = 0.70f;
};

struct realtime_relational_auditory_salience_result {
    realtime_musical_role_hypotheses hypotheses{};
    float spectral_distinctiveness = 0.0f;
    std::size_t competitor_count = 0;
};

inline bool valid_relational_salience_policy(
    const realtime_relational_auditory_salience_policy& policy) noexcept {
    return std::isfinite(policy.active_threshold) &&
        policy.active_threshold >= 0.0f && policy.active_threshold <= 1.0f &&
        std::isfinite(policy.full_continuity_age_seconds) &&
        policy.full_continuity_age_seconds > 0.0f &&
        std::isfinite(policy.confidence_cap) &&
        policy.confidence_cap >= 0.0f && policy.confidence_cap <= 1.0f;
}

inline bool realtime_coarse_profile_cosine_similarity(
    const realtime_source_spatial_observation& first,
    const realtime_source_spatial_observation& second,
    float& similarity) noexcept {
    double dot = 0.0;
    double first_norm = 0.0;
    double second_norm = 0.0;
    for (std::size_t band = 0; band < first.coarse_band_energy_share.size(); ++band) {
        const float a = first.coarse_band_energy_share[band];
        const float b = second.coarse_band_energy_share[band];
        if (!std::isfinite(a) || !std::isfinite(b) || a < 0.0f || b < 0.0f)
            return false;
        dot += static_cast<double>(a) * static_cast<double>(b);
        first_norm += static_cast<double>(a) * static_cast<double>(a);
        second_norm += static_cast<double>(b) * static_cast<double>(b);
    }
    if (!(first_norm > std::numeric_limits<double>::epsilon()) ||
        !(second_norm > std::numeric_limits<double>::epsilon())) {
        return false;
    }
    similarity = clamp_unit_interval(static_cast<float>(
        dot / std::sqrt(first_norm * second_norm)));
    return true;
}

inline realtime_relational_auditory_salience_result
propose_relational_auditory_salience(
    const spatial_source_evidence& source,
    const realtime_source_spatial_observation* observations,
    std::size_t observation_count,
    realtime_relational_auditory_salience_policy policy = {}) noexcept {
    realtime_relational_auditory_salience_result result{};
    if (!valid_relational_salience_policy(policy) ||
        (observation_count != 0 && observations == nullptr)) {
        return result;
    }

    const realtime_source_spatial_observation* target = nullptr;
    for (std::size_t index = 0; index < observation_count; ++index) {
        const auto& candidate = observations[index];
        if (candidate.source_id != source.source_id ||
            candidate.generation != source.generation) {
            continue;
        }
        // Ambiguous duplicate observations for one source identity are not
        // silently merged into a stronger perceptual claim.
        if (target != nullptr)
            return result;
        target = &candidate;
    }

    if (target == nullptr || !target->audio_observed ||
        target->lane_kind != spatial_audio_lane_kind::dry_source ||
        !std::isfinite(target->availability_fraction) ||
        !std::isfinite(target->activity) ||
        !std::isfinite(target->relative_energy) ||
        !std::isfinite(target->edge_ratio) ||
        !std::isfinite(target->source_age_seconds) ||
        target->availability_fraction <= 0.0f ||
        target->activity < policy.active_threshold ||
        target->source_age_seconds < 0.0f) {
        return result;
    }

    double distinctiveness_weighted_sum = 0.0;
    double competitor_weight_sum = 0.0;
    float competitor_availability = 1.0f;

    for (std::size_t index = 0; index < observation_count; ++index) {
        const auto& competitor = observations[index];
        if (&competitor == target || !competitor.audio_observed ||
            competitor.lane_kind != spatial_audio_lane_kind::dry_source ||
            (competitor.source_id == target->source_id &&
             competitor.generation == target->generation) ||
            !std::isfinite(competitor.activity) ||
            !std::isfinite(competitor.relative_energy) ||
            !std::isfinite(competitor.availability_fraction) ||
            competitor.activity < policy.active_threshold ||
            competitor.relative_energy <= 0.0f ||
            competitor.availability_fraction <= 0.0f) {
            continue;
        }

        float similarity = 0.0f;
        if (!realtime_coarse_profile_cosine_similarity(*target, competitor, similarity))
            continue;

        const double weight = static_cast<double>(
            clamp_unit_interval(competitor.relative_energy));
        if (!(weight > 0.0))
            continue;
        distinctiveness_weighted_sum +=
            weight * static_cast<double>(1.0f - similarity);
        competitor_weight_sum += weight;
        competitor_availability = std::min(
            competitor_availability,
            clamp_unit_interval(competitor.availability_fraction));
        ++result.competitor_count;
    }

    if (result.competitor_count == 0 || !(competitor_weight_sum > 0.0))
        return result;

    result.spectral_distinctiveness = clamp_unit_interval(static_cast<float>(
        distinctiveness_weighted_sum / competitor_weight_sum));

    const float relative_energy = clamp_unit_interval(target->relative_energy);
    const float activity = clamp_unit_interval(target->activity);
    const float edge_density = clamp_unit_interval(target->edge_ratio);

    // Energy remains the dominant term because this is salience relative to a
    // contemporaneous scene, not a timbre classifier. Spectral separation and
    // edge structure can distinguish a perceptually exposed line from an equally
    // energetic source embedded in the same broad spectral territory.
    const float salience = clamp_unit_interval(
        0.55f * relative_energy +
        0.20f * activity +
        0.15f * result.spectral_distinctiveness +
        0.10f * edge_density);

    const float continuity = clamp_unit_interval(
        target->source_age_seconds / policy.full_continuity_age_seconds);
    const float measurement_coverage = std::min(
        clamp_unit_interval(target->availability_fraction),
        competitor_availability);
    const float confidence = clamp_unit_interval(
        policy.confidence_cap * continuity * measurement_coverage);
    if (!(confidence > 0.0f))
        return result;

    result.hypotheses.foreground = {
        salience,
        confidence,
        role_cue_mask(realtime_musical_role_cue::relative_energy)
            | role_cue_mask(realtime_musical_role_cue::activity)
            | role_cue_mask(realtime_musical_role_cue::edge_density)
            | role_cue_mask(realtime_musical_role_cue::source_continuity)
            | role_cue_mask(realtime_musical_role_cue::spectral_contrast),
    };
    return result;
}

} // namespace vgmtooling::model
