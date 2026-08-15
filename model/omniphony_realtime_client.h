#pragma once

#include "omniphony_source_transport.h"

#include <cstddef>
#include <cstdint>

namespace vgmtooling::model {

struct omniphony_realtime_process_result {
    bool transport_valid = false;
    std::int32_t renderer_status = -1;
};

// Allocation-free caller for a host-resolved Omniphony source_ffi ABI 0.3
// instance. Dynamic-library loading stays outside the portable model layer;
// once the host resolves the opaque processor and three function pointers, the
// audio callback no longer needs to know the transport details.
template <std::size_t MaxLanes = 64, std::size_t MaxEvents = 256>
class omniphony_realtime_client {
public:
    bool bind(
        omniphony_source_processor_handle* processor,
        omniphony_source_abi_version_fn abi_major,
        omniphony_source_abi_version_fn abi_minor,
        omniphony_source_process_events_f32_fn process_events) noexcept
    {
        unbind();
        if (processor == nullptr || abi_major == nullptr || abi_minor == nullptr ||
            process_events == nullptr)
            return false;
        if (abi_major() != omniphony_source_abi_major_required ||
            abi_minor() < omniphony_source_abi_minor_required)
            return false;

        processor_ = processor;
        process_events_ = process_events;
        return true;
    }

    void unbind() noexcept {
        processor_ = nullptr;
        process_events_ = nullptr;
        transport_.reset();
    }

    bool bound() const noexcept {
        return processor_ != nullptr && process_events_ != nullptr;
    }

    omniphony_realtime_process_result process(
        const spatial_source_block_view& projected_block,
        float* interleaved_source_scratch,
        std::size_t interleaved_source_capacity,
        float* stereo_output,
        std::size_t stereo_output_capacity,
        std::uint64_t absolute_sample_position,
        std::uint32_t ramp_frames,
        const std::uint8_t* route_gain_preapplied = nullptr,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        omniphony_realtime_process_result result{};
        if (!bound())
            return result;
        if (!transport_.build(
                projected_block,
                route_gain_preapplied,
                persistent_part_min_confidence))
            return result;

        result.transport_valid = true;
        if (projected_block.frame_count != 0 &&
            (interleaved_source_scratch == nullptr || stereo_output == nullptr)) {
            result.renderer_status = -2;
            return result;
        }
        if (transport_.lane_count() != 0 &&
            projected_block.frame_count > interleaved_source_capacity / transport_.lane_count()) {
            result.renderer_status = -3;
            return result;
        }
        if (projected_block.frame_count > stereo_output_capacity / 2u) {
            result.renderer_status = -3;
            return result;
        }
        if (!transport_.interleave_pcm(
                projected_block,
                interleaved_source_scratch,
                interleaved_source_capacity)) {
            result.transport_valid = false;
            result.renderer_status = -4;
            return result;
        }

        result.renderer_status = process_events_(
            processor_,
            interleaved_source_scratch,
            transport_.lanes(),
            transport_.lane_count(),
            transport_.events(),
            transport_.event_count(),
            transport_.frame_count(),
            absolute_sample_position,
            ramp_frames,
            stereo_output);
        return result;
    }

    const omniphony_source_transport_storage<MaxLanes, MaxEvents>& transport() const noexcept {
        return transport_;
    }

private:
    omniphony_source_processor_handle* processor_ = nullptr;
    omniphony_source_process_events_f32_fn process_events_ = nullptr;
    omniphony_source_transport_storage<MaxLanes, MaxEvents> transport_{};
};

} // namespace vgmtooling::model
