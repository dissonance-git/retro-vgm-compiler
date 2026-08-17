#pragma once

#include "spc_native_exact_source_storage.h"
#include "spc_runtime_spatial_adapter.h"
#include "../../model/spatial_source_host_session.h"

#include <array>
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
    native_source_rejected,
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
    using native_storage_type = spc_native_exact_source_storage<CapacityFrames>;

    void reset(
        vgmtooling::model::spatial_source_host_discontinuity reason,
        std::uint64_t next_reference_frame = 0) noexcept
    {
        spatial_adapter_.reset();
        native_storage_.reset();
        host_session_.reset(reason, next_reference_frame);
        clear_errors();
    }

    void deactivate() noexcept {
        spatial_adapter_.reset();
        native_storage_.reset();
        host_session_.deactivate();
        clear_errors();
    }

    // Evidence-only path. This remains valid at any protected output rate for
    // which the runtime tick mapping is exact, even when dry PCM cannot yet be
    // aligned to the host-rate window.
    bool consume_reference_window(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        const spc_runtime_spatial_capture_view& capture) noexcept
    {
        clear_errors();
        if (!validate_reference_window(reference_frame_start, reference_frame_count))
            return false;
        if (!build_spatial_window(
                capture,
                window_start_tick,
                tick_rate,
                sample_rate,
                reference_frame_count))
            return false;
        return push_spatial_segments(
            reference_frame_start,
            reference_frame_count,
            nullptr);
    }

    // Exact dry-PCM path. It is intentionally narrower than the evidence path:
    // today it only admits a one-to-one 32 kHz protected reference window. At a
    // resampled host rate libgme's FIR phase/history is not reconstructed here,
    // so callers must use consume_reference_window() and leave PCM unavailable.
    bool consume_native_reference_window(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        const spc_runtime_spatial_capture_view& runtime_capture,
        const spc_native_source_capture& native_capture,
        std::uint64_t expected_native_sample_start) noexcept
    {
        clear_errors();
        if (!validate_reference_window(reference_frame_start, reference_frame_count))
            return false;

        // Native PCM is validated before the persistent runtime-spatial state is
        // allowed to move. A bad source capture therefore cannot consume runtime
        // trace ordinals or increment physical-voice generations.
        if (!native_storage_.build(
                native_capture,
                static_cast<std::uint32_t>(sample_rate),
                reference_frame_start,
                expected_native_sample_start,
                reference_frame_count)) {
            last_native_error_ = native_storage_.last_error();
            return fail(spc_runtime_host_pipeline_error::native_source_rejected);
        }

        if (!build_spatial_window(
                runtime_capture,
                window_start_tick,
                tick_rate,
                sample_rate,
                reference_frame_count))
            return false;

        return push_spatial_segments(
            reference_frame_start,
            reference_frame_count,
            &native_storage_);
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
    spc_native_exact_source_error last_native_error() const noexcept {
        return last_native_error_;
    }
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
    bool validate_reference_window(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count) noexcept
    {
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
        return true;
    }

    bool build_spatial_window(
        const spc_runtime_spatial_capture_view& capture,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        std::size_t reference_frame_count) noexcept
    {
        if (!spatial_adapter_.build_window(
                capture,
                window_start_tick,
                tick_rate,
                sample_rate,
                reference_frame_count)) {
            last_spatial_error_ = spatial_adapter_.last_error();
            return fail(spc_runtime_host_pipeline_error::spatial_adapter_rejected);
        }
        return true;
    }

    bool push_spatial_segments(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        const native_storage_type* native_audio) noexcept
    {
        const auto* segments = spatial_adapter_.segments();
        const std::size_t segment_count = spatial_adapter_.segment_count();
        for (std::size_t index = 0; index < segment_count; ++index) {
            const auto& segment = segments[index];
            const std::uint64_t segment_reference_frame =
                reference_frame_start + static_cast<std::uint64_t>(segment.reference_frame_offset);

            const vgmtooling::model::spatial_source_block_view* block = &segment.sources;
            vgmtooling::model::spatial_source_block_view decorated_block{};
            std::array<vgmtooling::model::spatial_audio_lane_view, 8> decorated_lanes{};
            if (native_audio != nullptr) {
                if (!native_audio->valid() || segment.sources.lane_count != 8 ||
                    segment.reference_frame_offset > native_audio->frame_count() ||
                    segment.sources.frame_count >
                        native_audio->frame_count() - segment.reference_frame_offset) {
                    return flush_after_host_failure(
                        reference_frame_start,
                        reference_frame_count,
                        spc_runtime_host_pipeline_error::native_source_rejected);
                }

                for (std::size_t voice = 0; voice < 8; ++voice) {
                    decorated_lanes[voice] = segment.sources.lanes[voice];
                    decorated_lanes[voice].mono_pcm =
                        native_audio->lane(voice) + segment.reference_frame_offset;
                    decorated_lanes[voice].availability =
                        native_audio->availability() + segment.reference_frame_offset;
                }
                decorated_block = segment.sources;
                decorated_block.lanes = decorated_lanes.data();
                block = &decorated_block;
            }

            if (!host_session_.push_at(segment_reference_frame, *block)) {
                last_host_error_ = host_session_.last_error();
                return flush_after_host_failure(
                    reference_frame_start,
                    reference_frame_count,
                    spc_runtime_host_pipeline_error::host_session_rejected);
            }
        }

        // A successful adapter window must cover the entire reference render.
        // The host session's exact reference cursor is a final executable check.
        const std::uint64_t expected_end =
            reference_frame_start + static_cast<std::uint64_t>(reference_frame_count);
        if (host_session_.expected_push_frame() != expected_end) {
            return flush_after_host_failure(
                reference_frame_start,
                reference_frame_count,
                spc_runtime_host_pipeline_error::host_session_rejected);
        }

        return true;
    }

    bool flush_after_host_failure(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        spc_runtime_host_pipeline_error error) noexcept
    {
        // The protected audio window has already happened, but its semantic
        // transport did not complete. Fail closed: drop any partial semantic
        // window and require a fresh exact runtime state after this boundary.
        const std::uint64_t next_reference_frame =
            reference_frame_start + static_cast<std::uint64_t>(reference_frame_count);
        spatial_adapter_.reset();
        native_storage_.reset();
        host_session_.reset(
            vgmtooling::model::spatial_source_host_discontinuity::decoder_flush,
            next_reference_frame);
        return fail(error);
    }

    void clear_errors() noexcept {
        last_error_ = spc_runtime_host_pipeline_error::none;
        last_native_error_ = spc_native_exact_source_error::none;
        last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
        last_host_error_ = vgmtooling::model::spatial_source_host_session_error::none;
    }

    bool fail(spc_runtime_host_pipeline_error error) noexcept {
        last_error_ = error;
        return false;
    }

    spatial_adapter_type spatial_adapter_{};
    native_storage_type native_storage_{};
    host_session_type host_session_{};
    spc_runtime_host_pipeline_error last_error_ = spc_runtime_host_pipeline_error::none;
    spc_native_exact_source_error last_native_error_ = spc_native_exact_source_error::none;
    spc_runtime_spatial_adapter_error last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
    vgmtooling::model::spatial_source_host_session_error last_host_error_ =
        vgmtooling::model::spatial_source_host_session_error::none;
};

} // namespace gameaudio::spc
