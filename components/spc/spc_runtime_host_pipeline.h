#pragma once

#include "spc_runtime_spatial_adapter.h"
#include "../../model/spatial_source_host_session.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

enum class spc_runtime_host_pipeline_error : std::uint8_t {
    none = 0,
    inactive,
    undrained_reference_window,
    noncontiguous_reference_window,
    reference_timeline_overflow,
    reference_window_too_large,
    spatial_adapter_rejected,
    host_session_rejected,
};

// Window-at-a-time bridge intended for a protected decoder call such as
// Spc_Emu::play(). One reference render window may be buffered at a time, while
// spatial_source_host_session may still expose that window to a downstream host
// in arbitrary smaller chunks.
//
// This shape deliberately mirrors the historical foo_gep SPC decoder lifecycle:
// a protected emulator render produces one stereo reference window; exact S-DSP
// observations captured during that call are projected beside it. The reference
// mix remains authoritative/audible. This class owns no foobar2000 API objects.
template <std::size_t CapacityFrames = 4096,
          std::size_t MaxSegments = 64,
          std::size_t MaxEvidenceEvents = 512,
          std::size_t HostMaxEvents = MaxEvidenceEvents + MaxSegments * 8u>
class spc_runtime_host_pipeline {
    static_assert(CapacityFrames > 0, "CapacityFrames must be non-zero");
    static_assert(MaxSegments > 0, "MaxSegments must be non-zero");
    static_assert(MaxEvidenceEvents > 0, "MaxEvidenceEvents must be non-zero");
    static_assert(
        HostMaxEvents >= MaxEvidenceEvents + (MaxSegments > 0 ? (MaxSegments - 1u) * 8u : 0u),
        "HostMaxEvents must cover timed evidence plus worst-case identity boundaries");

public:
    using spatial_adapter_type =
        spc_runtime_spatial_adapter<MaxSegments, MaxEvidenceEvents>;
    using host_session_type =
        vgmtooling::model::spatial_source_host_session<8, CapacityFrames, HostMaxEvents>;

    void reset(
        vgmtooling::model::spatial_source_host_discontinuity reason,
        std::uint64_t next_reference_frame = 0) noexcept
    {
        spatial_adapter_.reset();
        host_session_.reset(reason, next_reference_frame);
        last_error_ = spc_runtime_host_pipeline_error::none;
        last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
        last_host_error_ = vgmtooling::model::spatial_source_host_session_error::none;
    }

    void deactivate() noexcept {
        spatial_adapter_.reset();
        host_session_.deactivate();
        last_error_ = spc_runtime_host_pipeline_error::none;
        last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
        last_host_error_ = vgmtooling::model::spatial_source_host_session_error::none;
    }

    // Call once after the protected reference renderer has produced the matching
    // audio window and the realtime S-DSP capture for that same window is closed.
    // No semantic state is accepted for a different reference position.
    bool consume_reference_window(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        const spc_runtime_spatial_capture_view& capture) noexcept
    {
        clear_errors();

        if (!host_session_.active())
            return fail(spc_runtime_host_pipeline_error::inactive);
        if (host_session_.buffered_frames() != 0)
            return fail(spc_runtime_host_pipeline_error::undrained_reference_window);
        if (reference_frame_start != host_session_.expected_push_frame())
            return fail(spc_runtime_host_pipeline_error::noncontiguous_reference_window);
        if (reference_frame_count > CapacityFrames)
            return fail(spc_runtime_host_pipeline_error::reference_window_too_large);
        if (reference_frame_count >
            std::numeric_limits<std::uint64_t>::max() - reference_frame_start)
            return fail(spc_runtime_host_pipeline_error::reference_timeline_overflow);

        if (!spatial_adapter_.build_window(
                capture,
                window_start_tick,
                tick_rate,
                sample_rate,
                reference_frame_count)) {
            last_spatial_error_ = spatial_adapter_.last_error();
            return fail(spc_runtime_host_pipeline_error::spatial_adapter_rejected);
        }

        const auto* segments = spatial_adapter_.segments();
        const std::size_t segment_count = spatial_adapter_.segment_count();
        for (std::size_t index = 0; index < segment_count; ++index) {
            const auto& segment = segments[index];
            const std::uint64_t segment_reference_frame =
                reference_frame_start + static_cast<std::uint64_t>(segment.reference_frame_offset);
            if (!host_session_.push_at(segment_reference_frame, segment.sources)) {
                last_host_error_ = host_session_.last_error();

                // The protected audio window has already happened, but its
                // semantic transport did not. Fail closed: drop any partial
                // semantic window and require a fresh exact runtime state after
                // this decoder boundary rather than stitching a false history.
                const std::uint64_t next_reference_frame =
                    reference_frame_start + static_cast<std::uint64_t>(reference_frame_count);
                spatial_adapter_.reset();
                host_session_.reset(
                    vgmtooling::model::spatial_source_host_discontinuity::decoder_flush,
                    next_reference_frame);
                return fail(spc_runtime_host_pipeline_error::host_session_rejected);
            }
        }

        // A successful adapter window must cover the entire reference render.
        // The host session's exact reference cursor is a final executable check.
        const std::uint64_t expected_end =
            reference_frame_start + static_cast<std::uint64_t>(reference_frame_count);
        if (host_session_.expected_push_frame() != expected_end) {
            spatial_adapter_.reset();
            host_session_.reset(
                vgmtooling::model::spatial_source_host_discontinuity::decoder_flush,
                expected_end);
            return fail(spc_runtime_host_pipeline_error::host_session_rejected);
        }

        return true;
    }

    vgmtooling::model::spatial_source_host_chunk pull(std::size_t max_frames) noexcept {
        return host_session_.pull(max_frames);
    }

    bool active() const noexcept { return host_session_.active(); }
    std::size_t buffered_frames() const noexcept { return host_session_.buffered_frames(); }
    std::uint64_t session_epoch() const noexcept { return host_session_.session_epoch(); }
    std::uint64_t expected_reference_frame() const noexcept {
        return host_session_.expected_push_frame();
    }
    const spc_runtime_spatial_state& spatial_state() const noexcept {
        return spatial_adapter_.state();
    }
    spc_runtime_host_pipeline_error last_error() const noexcept { return last_error_; }
    spc_runtime_spatial_adapter_error last_spatial_error() const noexcept {
        return last_spatial_error_;
    }
    vgmtooling::model::spatial_source_host_session_error last_host_error() const noexcept {
        return last_host_error_;
    }
    bool last_pull_identity_limited() const noexcept {
        return host_session_.last_pull_identity_limited();
    }

private:
    void clear_errors() noexcept {
        last_error_ = spc_runtime_host_pipeline_error::none;
        last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
        last_host_error_ = vgmtooling::model::spatial_source_host_session_error::none;
    }

    bool fail(spc_runtime_host_pipeline_error error) noexcept {
        last_error_ = error;
        return false;
    }

    spatial_adapter_type spatial_adapter_{};
    host_session_type host_session_{};
    spc_runtime_host_pipeline_error last_error_ = spc_runtime_host_pipeline_error::none;
    spc_runtime_spatial_adapter_error last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
    vgmtooling::model::spatial_source_host_session_error last_host_error_ =
        vgmtooling::model::spatial_source_host_session_error::none;
};

} // namespace gameaudio::spc
