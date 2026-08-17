#pragma once

#include "../../model/realtime_musical_omniphony_pipeline.h"
#include "../../model/spatial_source_host_session.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

struct spc_realtime_musical_omniphony_result {
    bool source_chunk_valid = false;
    vgmtooling::model::realtime_musical_omniphony_result omniphony{};
};

// SPC-specific front door from the validated source host pipeline to Omniphony.
// The upstream SPC host has already decided which exact dry waveform exists for
// each voice (reference, reconstructed, or verified upstream source) and has
// already preserved the shared wet return. This class only presents that causal
// source block. It has no source-quality switch and cannot influence synthesis.
template <
    std::size_t MaxLanes = 10,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
class spc_realtime_musical_omniphony_pipeline {
public:
    using pipeline_type = vgmtooling::model::realtime_musical_omniphony_pipeline<
        MaxLanes,
        MaxEvents,
        RoleCapacity>;

    bool bind_renderer(
        vgmtooling::model::omniphony_source_processor_handle* processor,
        vgmtooling::model::omniphony_source_abi_version_fn abi_major,
        vgmtooling::model::omniphony_source_abi_version_fn abi_minor,
        vgmtooling::model::omniphony_source_reset_fn reset,
        vgmtooling::model::omniphony_source_process_events_f32_fn process_events) noexcept
    {
        return pipeline_.bind_renderer(
            processor,
            abi_major,
            abi_minor,
            reset,
            process_events);
    }

    void unbind_renderer() noexcept { pipeline_.unbind_renderer(); }
    bool renderer_bound() const noexcept { return pipeline_.renderer_bound(); }
    bool reset() noexcept { return pipeline_.reset(); }

    spc_realtime_musical_omniphony_result process_chunk(
        const vgmtooling::model::spatial_source_host_chunk& chunk,
        double sample_rate,
        float* interleaved_source_scratch,
        std::size_t interleaved_source_capacity,
        float* stereo_output,
        std::size_t stereo_output_capacity,
        std::uint32_t ramp_frames,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        spc_realtime_musical_omniphony_result result{};
        if (chunk.sources.frame_count == 0 || chunk.sources.lanes == nullptr ||
            chunk.sources.lane_count == 0 || chunk.sources.lane_count > MaxLanes)
            return result;

        result.source_chunk_valid = true;
        result.omniphony = pipeline_.process_block(
            chunk.sources,
            sample_rate,
            interleaved_source_scratch,
            interleaved_source_capacity,
            stereo_output,
            stereo_output_capacity,
            chunk.reference_frame_start,
            ramp_frames,
            nullptr,
            persistent_part_min_confidence);
        return result;
    }

    const pipeline_type& pipeline() const noexcept { return pipeline_; }

private:
    pipeline_type pipeline_{};
};

} // namespace gameaudio::spc
