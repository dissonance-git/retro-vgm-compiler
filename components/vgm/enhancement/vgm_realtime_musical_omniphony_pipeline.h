#pragma once

#include "timed_spatial_source_bus.h"
#include "../../../model/realtime_musical_omniphony_pipeline.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct vgm_realtime_musical_omniphony_result {
    bool source_block_valid = false;
    vgmtooling::model::realtime_musical_omniphony_result omniphony{};
};

// Chip-neutral VGM front door to the causal Omniphony pipeline. Source quality
// is deliberately absent from this API: callers pass exact isolated source lanes
// that were already selected by their device-family admission/fallback logic.
template <
    std::size_t SourceCount,
    std::size_t MaxFrames = 8192,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
class vgm_realtime_musical_omniphony_pipeline {
    static_assert(SourceCount > 0, "VGM source count must be non-zero");

public:
    static constexpr std::size_t source_capacity = SourceCount;
    using source_array = std::array<source_family_stereo_view, source_capacity>;
    using evidence_array =
        std::array<vgmtooling::model::spatial_source_evidence, source_capacity>;
    using spatial_bus_type =
        timed_spatial_source_bus_storage<source_capacity, MaxFrames, MaxEvents>;
    using pipeline_type = vgmtooling::model::realtime_musical_omniphony_pipeline<
        source_capacity,
        MaxEvents,
        RoleCapacity>;

    bool bind_renderer(
        vgmtooling::model::omniphony_source_processor_handle* processor,
        vgmtooling::model::omniphony_source_abi_version_fn abi_major,
        vgmtooling::model::omniphony_source_abi_version_fn abi_minor,
        vgmtooling::model::omniphony_source_reset_fn reset,
        vgmtooling::model::omniphony_source_set_mix_budget_fn set_mix_budget,
        vgmtooling::model::omniphony_source_process_events_f32_fn process_events) noexcept
    {
        return pipeline_.bind_renderer(
            processor,
            abi_major,
            abi_minor,
            reset,
            set_mix_budget,
            process_events);
    }

    void unbind_renderer() noexcept { pipeline_.unbind_renderer(); }
    bool renderer_bound() const noexcept { return pipeline_.renderer_bound(); }

    bool reset() noexcept {
        spatial_bus_.reset();
        return pipeline_.reset();
    }

    vgm_realtime_musical_omniphony_result process_selected_sources(
        const source_array& selected_sources,
        const evidence_array& evidence,
        std::size_t frame_count,
        double sample_rate,
        float* interleaved_source_scratch,
        std::size_t interleaved_source_capacity,
        float* stereo_output,
        std::size_t stereo_output_capacity,
        std::uint64_t absolute_sample_position,
        std::uint32_t ramp_frames,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        return process_selected_sources_timed(
            selected_sources,
            evidence,
            nullptr,
            0,
            frame_count,
            sample_rate,
            interleaved_source_scratch,
            interleaved_source_capacity,
            stereo_output,
            stereo_output_capacity,
            absolute_sample_position,
            ramp_frames,
            persistent_part_min_confidence);
    }

    vgm_realtime_musical_omniphony_result process_selected_sources_timed(
        const source_array& selected_sources,
        const evidence_array& initial_evidence,
        const vgmtooling::model::spatial_source_evidence_event* evidence_events,
        std::size_t evidence_event_count,
        std::size_t frame_count,
        double sample_rate,
        float* interleaved_source_scratch,
        std::size_t interleaved_source_capacity,
        float* stereo_output,
        std::size_t stereo_output_capacity,
        std::uint64_t absolute_sample_position,
        std::uint32_t ramp_frames,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        vgm_realtime_musical_omniphony_result result{};
        if (!spatial_bus_.build(
                selected_sources,
                initial_evidence,
                frame_count,
                evidence_events,
                evidence_event_count))
            return result;

        result.source_block_valid = true;
        result.omniphony = pipeline_.process_block(
            spatial_bus_.block(),
            sample_rate,
            interleaved_source_scratch,
            interleaved_source_capacity,
            stereo_output,
            stereo_output_capacity,
            absolute_sample_position,
            ramp_frames,
            nullptr,
            persistent_part_min_confidence);
        return result;
    }

    const spatial_bus_type& spatial_bus() const noexcept { return spatial_bus_; }
    const pipeline_type& pipeline() const noexcept { return pipeline_; }

private:
    spatial_bus_type spatial_bus_{};
    pipeline_type pipeline_{};
};

} // namespace gameaudio::vgm
