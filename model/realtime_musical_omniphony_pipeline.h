#pragma once

#include "omniphony_realtime_client.h"
#include "realtime_musical_spatial_frontend.h"

#include <cstddef>
#include <cstdint>

namespace vgmtooling::model {

struct realtime_musical_omniphony_result {
    bool prepared = false;
    bool rendered = false;
    bool learned = false;
    bool transport_valid = false;
    std::int32_t renderer_status = -1;
};

// Complete causal runtime seam from raw source evidence to Omniphony and back
// into future musical memory. The ordering is intentionally encapsulated:
//
//   raw current block
//       -> prepare past-only presentation
//       -> render projected view through Omniphony
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

    bool bind_renderer(
        omniphony_source_processor_handle* processor,
        omniphony_source_abi_version_fn abi_major,
        omniphony_source_abi_version_fn abi_minor,
        omniphony_source_reset_fn reset,
        omniphony_source_process_events_f32_fn process_events) noexcept
    {
        return client_.bind(processor, abi_major, abi_minor, reset, process_events);
    }

    void unbind_renderer() noexcept {
        client_.unbind();
    }

    bool renderer_bound() const noexcept {
        return client_.bound();
    }

    // Track changes, seeks and decoder restarts must clear both halves of the
    // causal state machine. Resetting only GMI would leave old Omniphony pose /
    // source-identity history alive; resetting only Omniphony would leave old
    // musical memory steering a fresh timeline.
    bool reset() noexcept {
        frontend_.reset();
        handoff_.reset();
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

        const omniphony_realtime_process_result render_result = client_.process(
            handoff_.projected_view(),
            interleaved_source_scratch,
            interleaved_source_capacity,
            stereo_output,
            stereo_output_capacity,
            absolute_sample_position,
            ramp_frames,
            route_gain_preapplied,
            persistent_part_min_confidence);
        result.transport_valid = render_result.transport_valid;
        result.renderer_status = render_result.renderer_status;
        if (render_result.renderer_status != 0)
            return result;

        result.rendered = true;

        // Learn only after the block was accepted by the renderer. On a render
        // failure the caller may retry or fall back without having advanced
        // musical memory behind its back. complete_block itself keeps its
        // documented fail-closed semantics if post-render observation fails.
        result.learned = frontend_.complete_block(raw_block, sample_rate);
        return result;
    }

    const frontend_type& frontend() const noexcept { return frontend_; }
    const client_type& client() const noexcept { return client_; }
    const handoff_storage& handoff() const noexcept { return handoff_; }

private:
    frontend_type frontend_{};
    client_type client_{};
    handoff_storage handoff_{};
};

} // namespace vgmtooling::model
