#pragma once

#include "spatial_source.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

// Lightweight acoustic observations intended for online musical reasoning.
// They describe what was measured in a source lane, not what musical role the
// source "must" have and not where a renderer should place it.
struct realtime_source_spatial_observation {
    bool audio_observed = false;
    std::uint64_t source_id = 0;
    std::uint64_t generation = 0;
    spatial_audio_lane_kind lane_kind = spatial_audio_lane_kind::dry_source;

    std::size_t observed_frames = 0;
    float availability_fraction = 0.0f;
    float rms = 0.0f;
    float peak = 0.0f;
    float activity = 0.0f;
    float relative_energy = 0.0f;

    // Energy below the configured one-pole analysis cutoff divided by total
    // observed energy. This is a cheap causal low-register/body proxy, not a
    // pitch detector and not an instrument classifier.
    float low_band_energy_ratio = 0.0f;

    // Mean absolute sample-to-sample change relative to mean absolute signal.
    // Useful as a bounded edge/transient proxy, but deliberately not called
    // "percussion" or "foreground" because those are higher musical claims.
    float edge_ratio = 0.0f;

    // Time for which this exact bounded source identity has occupied the lane.
    // It survives ordinary block boundaries but resets on source/generation
    // change. This is structural continuity, not proof of a persistent part.
    float source_age_seconds = 0.0f;
};

struct realtime_scene_spatial_observation {
    bool audio_observed = false;
    std::size_t observed_lane_count = 0;
    std::size_t active_lane_count = 0;

    float mean_activity = 0.0f;

    // Sum(p_i^2) over per-lane energy shares. Values near 1 mean one lane owns
    // most observed energy; lower values mean energy is distributed. The value
    // is descriptive only and is not itself a width or diffusion control.
    float energy_concentration = 0.0f;

    float low_band_energy_ratio = 0.0f;
    float edge_ratio = 0.0f;
    float shared_effect_energy_share = 0.0f;
};

struct realtime_musical_spatial_observer_config {
    float low_band_cutoff_hz = 300.0f;
    float activity_reference_rms = 0.035f;
    float active_threshold = 0.15f;
};

template <std::size_t MaxLanes = 64, std::size_t MaxEvents = 256>
class realtime_musical_spatial_observer {
public:
    static_assert(MaxLanes > 0, "realtime observer needs at least one lane slot");

    void reset() noexcept {
        lanes_ = {};
        sources_ = {};
        scene_ = {};
        lane_count_ = 0;
    }

    const realtime_musical_spatial_observer_config& config() const noexcept {
        return config_;
    }

    bool set_config(realtime_musical_spatial_observer_config config) noexcept {
        if (!valid_config(config))
            return false;
        config_ = config;
        return true;
    }

    const realtime_source_spatial_observation& source(std::size_t lane_index) const noexcept {
        return sources_[lane_index];
    }

    const realtime_scene_spatial_observation& scene() const noexcept {
        return scene_;
    }

    std::size_t lane_count() const noexcept {
        return lane_count_;
    }

    // Analyze the current audio block only. The observer carries bounded filter
    // and identity state forward, but it never requests future samples or a
    // whole-song prepass. A caller may use smaller sub-blocks when lower control
    // latency is worth the additional analysis/control traffic.
    //
    // This block-summary contract intentionally refuses an in-block source-ID
    // change on one physical lane. A caller that needs that case must split the
    // audio at the identity boundary rather than merge two sources into one
    // acoustic observation.
    bool process(const spatial_source_block_view& block, double sample_rate) noexcept {
        if (!valid_config(config_) || !std::isfinite(sample_rate) || sample_rate <= 0.0 ||
            block.lane_count > MaxLanes || block.evidence_event_count > MaxEvents ||
            (block.lane_count != 0 && block.lanes == nullptr) ||
            (block.evidence_event_count != 0 && block.evidence_events == nullptr))
            return false;

        std::size_t previous_event_offset = 0;
        bool have_previous_event = false;
        for (std::size_t event_index = 0; event_index < block.evidence_event_count; ++event_index) {
            const spatial_source_evidence_event& event = block.evidence_events[event_index];
            if (event.lane_index >= block.lane_count || event.frame_offset > block.frame_count ||
                (have_previous_event && event.frame_offset < previous_event_offset))
                return false;

            have_previous_event = true;
            previous_event_offset = event.frame_offset;

            const spatial_source_evidence& initial = block.lanes[event.lane_index].evidence;
            if (event.evidence.source_id != initial.source_id ||
                event.evidence.generation != initial.generation)
                return false;
        }

        const double nyquist = sample_rate * 0.5;
        if (!(static_cast<double>(config_.low_band_cutoff_hz) < nyquist))
            return false;

        auto next_lanes = lanes_;
        std::array<realtime_source_spatial_observation, MaxLanes> next_sources{};
        std::array<double, MaxLanes> energy{};
        std::array<double, MaxLanes> low_energy{};
        std::array<double, MaxLanes> absolute_sum{};
        std::array<double, MaxLanes> edge_sum{};

        constexpr double pi = 3.141592653589793238462643383279502884;
        const double lowpass_memory = std::exp(
            -2.0 * pi * static_cast<double>(config_.low_band_cutoff_hz) / sample_rate);
        const double lowpass_input = 1.0 - lowpass_memory;

        for (std::size_t lane_index = 0; lane_index < block.lane_count; ++lane_index) {
            const spatial_audio_lane_view& lane = block.lanes[lane_index];
            lane_state& state = next_lanes[lane_index];
            realtime_source_spatial_observation& observation = next_sources[lane_index];

            if (!same_identity(state, lane.evidence)) {
                state = {};
                state.initialized = true;
                state.source_id = lane.evidence.source_id;
                state.generation = lane.evidence.generation;
            }
            state.age_frames = saturating_add(state.age_frames, block.frame_count);

            observation.source_id = lane.evidence.source_id;
            observation.generation = lane.evidence.generation;
            observation.lane_kind = lane.kind;
            observation.source_age_seconds = static_cast<float>(
                static_cast<double>(state.age_frames) / sample_rate);

            if (lane.mono_pcm == nullptr || block.frame_count == 0) {
                state.have_previous_sample = false;
                state.have_lowpass = false;
                continue;
            }

            double sum_square = 0.0;
            double sum_low_square = 0.0;
            double sum_absolute = 0.0;
            double sum_edge = 0.0;
            float peak = 0.0f;
            std::size_t observed = 0;

            for (std::size_t frame = 0; frame < block.frame_count; ++frame) {
                if (lane.availability != nullptr && lane.availability[frame] == 0) {
                    // Unknown audio cannot be advanced through the analysis
                    // filter as if it were silence. Break local continuity and
                    // restart from the next actually observed sample.
                    state.have_previous_sample = false;
                    state.have_lowpass = false;
                    continue;
                }

                const float sample = lane.mono_pcm[frame];
                if (!std::isfinite(sample)) {
                    state.have_previous_sample = false;
                    state.have_lowpass = false;
                    continue;
                }

                if (!state.have_lowpass) {
                    state.lowpass = static_cast<double>(sample);
                    state.have_lowpass = true;
                } else {
                    state.lowpass = lowpass_input * static_cast<double>(sample)
                        + lowpass_memory * state.lowpass;
                }

                const double value = static_cast<double>(sample);
                sum_square += value * value;
                sum_low_square += state.lowpass * state.lowpass;
                sum_absolute += std::fabs(value);
                peak = std::fmax(peak, std::fabs(sample));

                if (state.have_previous_sample)
                    sum_edge += std::fabs(value - static_cast<double>(state.previous_sample));
                state.previous_sample = sample;
                state.have_previous_sample = true;
                ++observed;
            }

            observation.observed_frames = observed;
            observation.availability_fraction = block.frame_count == 0
                ? 0.0f
                : clamp_unit_interval(static_cast<float>(observed)
                    / static_cast<float>(block.frame_count));

            if (observed == 0)
                continue;

            observation.audio_observed = true;
            observation.rms = static_cast<float>(
                std::sqrt(sum_square / static_cast<double>(observed)));
            observation.peak = peak;
            observation.activity = activity_from_rms(observation.rms, config_.activity_reference_rms);
            observation.low_band_energy_ratio = ratio(sum_low_square, sum_square);
            observation.edge_ratio = ratio(sum_edge, sum_absolute);

            energy[lane_index] = sum_square;
            low_energy[lane_index] = sum_low_square;
            absolute_sum[lane_index] = sum_absolute;
            edge_sum[lane_index] = sum_edge;
        }

        realtime_scene_spatial_observation next_scene{};
        double total_energy = 0.0;
        double total_low_energy = 0.0;
        double total_absolute = 0.0;
        double total_edge = 0.0;
        double shared_effect_energy = 0.0;
        double activity_sum = 0.0;

        for (std::size_t lane_index = 0; lane_index < block.lane_count; ++lane_index) {
            realtime_source_spatial_observation& observation = next_sources[lane_index];
            if (!observation.audio_observed)
                continue;

            ++next_scene.observed_lane_count;
            if (observation.activity >= config_.active_threshold)
                ++next_scene.active_lane_count;
            activity_sum += observation.activity;
            total_energy += energy[lane_index];
            total_low_energy += low_energy[lane_index];
            total_absolute += absolute_sum[lane_index];
            total_edge += edge_sum[lane_index];
            if (observation.lane_kind == spatial_audio_lane_kind::shared_effect_return)
                shared_effect_energy += energy[lane_index];
        }

        if (next_scene.observed_lane_count != 0) {
            next_scene.audio_observed = true;
            next_scene.mean_activity = clamp_unit_interval(static_cast<float>(
                activity_sum / static_cast<double>(next_scene.observed_lane_count)));
        }

        if (total_energy > 0.0) {
            double concentration = 0.0;
            for (std::size_t lane_index = 0; lane_index < block.lane_count; ++lane_index) {
                if (!next_sources[lane_index].audio_observed)
                    continue;
                const double share = energy[lane_index] / total_energy;
                next_sources[lane_index].relative_energy = clamp_unit_interval(
                    static_cast<float>(share));
                concentration += share * share;
            }
            next_scene.energy_concentration = clamp_unit_interval(static_cast<float>(concentration));
            next_scene.low_band_energy_ratio = ratio(total_low_energy, total_energy);
            next_scene.shared_effect_energy_share = ratio(shared_effect_energy, total_energy);
        }
        next_scene.edge_ratio = ratio(total_edge, total_absolute);

        for (std::size_t lane_index = block.lane_count; lane_index < MaxLanes; ++lane_index)
            next_lanes[lane_index] = {};

        lanes_ = next_lanes;
        sources_ = next_sources;
        scene_ = next_scene;
        lane_count_ = block.lane_count;
        return true;
    }

private:
    struct lane_state {
        bool initialized = false;
        std::uint64_t source_id = 0;
        std::uint64_t generation = 0;
        std::uint64_t age_frames = 0;
        double lowpass = 0.0;
        bool have_lowpass = false;
        float previous_sample = 0.0f;
        bool have_previous_sample = false;
    };

    static bool valid_config(const realtime_musical_spatial_observer_config& config) noexcept {
        return std::isfinite(config.low_band_cutoff_hz) && config.low_band_cutoff_hz > 0.0f &&
            std::isfinite(config.activity_reference_rms) && config.activity_reference_rms > 0.0f &&
            std::isfinite(config.active_threshold) && config.active_threshold >= 0.0f &&
            config.active_threshold <= 1.0f;
    }

    static bool same_identity(const lane_state& state, const spatial_source_evidence& evidence) noexcept {
        return state.initialized && state.source_id == evidence.source_id
            && state.generation == evidence.generation;
    }

    static std::uint64_t saturating_add(std::uint64_t left, std::size_t right) noexcept {
        const std::uint64_t amount = static_cast<std::uint64_t>(right);
        const std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
        return left > maximum - amount ? maximum : left + amount;
    }

    static float activity_from_rms(float rms, float reference) noexcept {
        if (!(rms > 0.0f))
            return 0.0f;
        return clamp_unit_interval(rms / (rms + reference));
    }

    static float ratio(double numerator, double denominator) noexcept {
        if (!(denominator > 0.0) || !(numerator > 0.0))
            return 0.0f;
        return clamp_unit_interval(static_cast<float>(numerator / denominator));
    }

    realtime_musical_spatial_observer_config config_{};
    std::array<lane_state, MaxLanes> lanes_{};
    std::array<realtime_source_spatial_observation, MaxLanes> sources_{};
    realtime_scene_spatial_observation scene_{};
    std::size_t lane_count_ = 0;
};

} // namespace vgmtooling::model
