#pragma once

#include "omniphony_realtime_client.h"
#include "realtime_musical_spatial_frontend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

struct realtime_musical_omniphony_result {
    bool prepared = false;
    bool budget_committed = false;
    bool rendered = false;
    bool learned = false;
    bool transport_valid = false;
    std::int32_t renderer_status = -1;
    // Non-zero only when this call produced a new completed governor trace.
    // A failed render can therefore never be mistaken for the previous call's
    // still-readable last_governor_trace().
    std::uint64_t governor_trace_index = 0;
};

struct realtime_spatial_overlap_pair_observation {
    bool valid = false;
    std::size_t left_lane_index = 0;
    std::size_t right_lane_index = 0;
    std::uint64_t left_source_id = 0;
    std::uint64_t left_generation = 0;
    std::uint64_t right_source_id = 0;
    std::uint64_t right_generation = 0;
    float coarse_spectral_overlap = 0.0f;
    float pair_energy_weight = 0.0f;
};

// Passive evidence snapshot for one successfully rendered-and-learned block.
// It records what the compiler knew before rendering, what actually crossed the
// ABI 0.4 scene-budget setter, and what the completed RAW source block taught
// the observer/governor for the future. It is diagnostic evidence only: nothing
// in this record is read back into the realtime control path.
template <std::size_t MaxLanes>
struct realtime_spatial_governor_trace {
    bool valid = false;
    // Monotonic within one pipeline timeline; reset() starts a fresh generation
    // at 1. This is diagnostic transaction identity, not musical/source identity.
    std::uint64_t sequence_index = 0;
    std::size_t lane_count = 0;
    std::size_t frame_count = 0;
    double sample_rate = 0.0;
    std::uint64_t absolute_sample_position = 0;
    float active_threshold = 0.0f;
    realtime_spatial_mix_budget applied_budget{};
    omniphony_source_mix_budget_v1_transport renderer_budget{};
    realtime_spatial_mix_budget learned_budget{};
    realtime_scene_spatial_observation scene{};
    std::array<realtime_source_spatial_observation, MaxLanes> sources{};

    // Compute one dry-source pair lazily from the already-recorded source
    // observations. No pair matrix is stored on the audio path. The energy
    // weight uses relative-energy shares; the common scene-energy denominator
    // cancels when pair weights are normalized, so this reconstructs the same
    // weighted scalar used by the observer.
    bool pair(
        std::size_t left_lane_index,
        std::size_t right_lane_index,
        realtime_spatial_overlap_pair_observation& out) const noexcept
    {
        out = {};
        if (!valid || left_lane_index >= lane_count || right_lane_index >= lane_count ||
            left_lane_index == right_lane_index)
            return false;
        if (right_lane_index < left_lane_index)
            std::swap(left_lane_index, right_lane_index);

        const auto& left = sources[left_lane_index];
        const auto& right = sources[right_lane_index];
        if (!left.audio_observed || !right.audio_observed ||
            left.lane_kind != spatial_audio_lane_kind::dry_source ||
            right.lane_kind != spatial_audio_lane_kind::dry_source ||
            left.activity < active_threshold || right.activity < active_threshold ||
            !(left.relative_energy > 0.0f) || !(right.relative_energy > 0.0f))
            return false;

        float overlap = 0.0f;
        for (std::size_t band = 0; band < left.coarse_band_energy_share.size(); ++band) {
            overlap += std::min(
                clamp_unit_interval(left.coarse_band_energy_share[band]),
                clamp_unit_interval(right.coarse_band_energy_share[band]));
        }
        const double weight = std::sqrt(
            static_cast<double>(left.relative_energy) *
            static_cast<double>(right.relative_energy));
        if (!(weight > 0.0) || !std::isfinite(weight))
            return false;

        out.valid = true;
        out.left_lane_index = left_lane_index;
        out.right_lane_index = right_lane_index;
        out.left_source_id = left.source_id;
        out.left_generation = left.generation;
        out.right_source_id = right.source_id;
        out.right_generation = right.generation;
        out.coarse_spectral_overlap = clamp_unit_interval(overlap);
        out.pair_energy_weight = clamp_unit_interval(static_cast<float>(weight));
        return true;
    }

    float reconstructed_coarse_spectral_overlap() const noexcept {
        if (!valid)
            return 0.0f;

        double weighted_overlap = 0.0;
        double total_pair_weight = 0.0;
        for (std::size_t left = 0; left < lane_count; ++left) {
            for (std::size_t right = left + 1; right < lane_count; ++right) {
                realtime_spatial_overlap_pair_observation observation{};
                if (!pair(left, right, observation))
                    continue;
                weighted_overlap += static_cast<double>(observation.pair_energy_weight) *
                    static_cast<double>(observation.coarse_spectral_overlap);
                total_pair_weight += static_cast<double>(observation.pair_energy_weight);
            }
        }
        if (!(total_pair_weight > 0.0))
            return 0.0f;
        return clamp_unit_interval(static_cast<float>(weighted_overlap / total_pair_weight));
    }
};

// Complete causal runtime seam from raw source evidence to Omniphony and back
// into future musical memory. The ordering is intentionally encapsulated:
//
//   raw current block
//       -> prepare past-only presentation + past-only scene mix budget
//       -> apply budget and render projected view through Omniphony
//       -> only after successful rendering, learn from the RAW completed block
//
// A caller therefore cannot accidentally let current PCM spatialize itself or
// feed yesterday's inferred presentation back as today's source observation.
template <
    std::size_t MaxLanes = 64,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
class realtime_musical_omniphony_pipeline {
public:
    using frontend_type =
        realtime_musical_spatial_frontend<MaxLanes, MaxEvents, RoleCapacity>;
    using client_type = omniphony_realtime_client<MaxLanes, MaxEvents>;
    using handoff_storage = typename frontend_type::handoff_storage;
    using governor_trace_type = realtime_spatial_governor_trace<MaxLanes>;

    bool bind_renderer(
        omniphony_source_processor_handle* processor,
        omniphony_source_abi_version_fn abi_major,
        omniphony_source_abi_version_fn abi_minor,
        omniphony_source_reset_fn reset,
        omniphony_source_set_mix_budget_fn set_mix_budget,
        omniphony_source_process_events_f32_fn process_events) noexcept
    {
        return client_.bind(
            processor,
            abi_major,
            abi_minor,
            reset,
            set_mix_budget,
            process_events);
    }

    void unbind_renderer() noexcept {
        client_.unbind();
    }

    bool renderer_bound() const noexcept {
        return client_.bound();
    }

    // Track changes, seeks and decoder restarts must clear both halves of the
    // causal state machine. Resetting only the compiler would leave old Omniphony
    // pose/source-identity/budget state alive; resetting only Omniphony would
    // leave old musical and scene memory steering a fresh timeline. Diagnostic
    // history belongs to that same timeline and is cleared as well.
    bool reset() noexcept {
        frontend_.reset();
        handoff_.reset();
        last_governor_trace_ = {};
        next_governor_trace_index_ = 1;
        return client_.reset_renderer();
    }

    realtime_musical_omniphony_result process_block(
        const spatial_source_block_view& raw_block,
        double sample_rate,
        float* interleaved_source_scratch,
        std::size_t interleaved_source_capacity,
        float* stereo_output,
        std::size_t stereo_output_capacity,
        std::uint64_t absolute_sample_position,
        std::uint32_t ramp_frames,
        const std::uint8_t* route_gain_preapplied = nullptr,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        realtime_musical_omniphony_result result{};
        if (!renderer_bound())
            return result;

        if (!frontend_.prepare_block(raw_block, handoff_))
            return result;
        result.prepared = true;

        // This is the budget learned only from blocks that have already sounded.
        // The current raw PCM cannot alter its own scene capacity.
        const realtime_spatial_mix_budget applied_budget = frontend_.mix_budget();
        const omniphony_realtime_process_result render_result = client_.process(
            handoff_.projected_view(),
            applied_budget,
            interleaved_source_scratch,
            interleaved_source_capacity,
            stereo_output,
            stereo_output_capacity,
            absolute_sample_position,
            ramp_frames,
            route_gain_preapplied,
            persistent_part_min_confidence);
        result.budget_committed = render_result.budget_committed;
        result.transport_valid = render_result.transport_valid;
        result.renderer_status = render_result.renderer_status;
        if (render_result.renderer_status != 0)
            return result;

        result.rendered = true;

        // Learn only after the block was accepted by the renderer. On a render
        // failure the caller may retry or fall back without semantic state or
        // the adaptive scene budget jumping ahead of the audio that sounded.
        result.learned = frontend_.complete_block(raw_block, sample_rate);
        if (!result.learned)
            return result;

        governor_trace_type trace{};
        trace.valid = true;
        trace.sequence_index = next_governor_trace_index_;
        trace.lane_count = frontend_.observer().lane_count();
        trace.frame_count = raw_block.frame_count;
        trace.sample_rate = sample_rate;
        trace.absolute_sample_position = absolute_sample_position;
        trace.active_threshold = frontend_.observer().config().active_threshold;
        trace.applied_budget = applied_budget;
        trace.renderer_budget = render_result.renderer_budget;
        trace.learned_budget = frontend_.mix_budget();
        trace.scene = frontend_.observer().scene();
        for (std::size_t lane_index = 0; lane_index < trace.lane_count; ++lane_index)
            trace.sources[lane_index] = frontend_.observer().source(lane_index);
        last_governor_trace_ = trace;
        result.governor_trace_index = trace.sequence_index;
        if (next_governor_trace_index_ != std::numeric_limits<std::uint64_t>::max())
            ++next_governor_trace_index_;
        return result;
    }

    const frontend_type& frontend() const noexcept { return frontend_; }
    const client_type& client() const noexcept { return client_; }
    const handoff_storage& handoff() const noexcept { return handoff_; }
    const governor_trace_type& last_governor_trace() const noexcept {
        return last_governor_trace_;
    }

private:
    frontend_type frontend_{};
    client_type client_{};
    handoff_storage handoff_{};
    governor_trace_type last_governor_trace_{};
    std::uint64_t next_governor_trace_index_ = 1;
};

} // namespace vgmtooling::model
