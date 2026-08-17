#pragma once

#include "spc_native_exact_source_storage.h"
#include "spc_runtime_spatial_adapter.h"
#include "snesapu_source_object_projection.h"
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
    snesapu_source_rejected,
    spatial_adapter_rejected,
    host_session_rejected,
};

// Window-at-a-time bridge intended for a protected decoder call such as
// Spc_Emu::play() or the editable SNESAPU shell. One reference render window may
// be buffered at a time, while spatial_source_host_session may still expose that
// window to a downstream host in arbitrary smaller chunks.
//
// The protected reference mix remains authoritative/audible. Source lanes are a
// parallel causal transport for analysis, enhanced rendering, and Omniphony.
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
        "HostMaxEvents must cover timed evidence plus worst-case dry-voice identity boundaries");

public:
    using spatial_adapter_type =
        spc_runtime_spatial_adapter<MaxSegments, MaxEvidenceEvents>;
    using host_session_type =
        vgmtooling::model::spatial_source_host_session<10, CapacityFrames, HostMaxEvents>;
    using native_storage_type = spc_native_exact_source_storage<CapacityFrames>;
    using snesapu_projection_type =
        snesapu_source_object_projection_storage<
            snesapu_source_transport_v2::max_frames,
            MaxEvidenceEvents>;

    void reset(
        vgmtooling::model::spatial_source_host_discontinuity reason,
        std::uint64_t next_reference_frame = 0) noexcept
    {
        spatial_adapter_.reset();
        native_storage_.reset();
        snesapu_projection_.reset();
        host_session_.reset(reason, next_reference_frame);
        clear_errors();
    }

    void deactivate() noexcept {
        spatial_adapter_.reset();
        native_storage_.reset();
        snesapu_projection_.reset();
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

    // Exact dry-PCM path for the native libgme-style 32 kHz hook. It is
    // intentionally narrower than the evidence path: at a resampled host rate
    // the reference FIR phase/history is not reconstructed here.
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

        // Native PCM is validated before persistent runtime-spatial state is
        // allowed to move. A bad source capture therefore cannot consume trace
        // ordinals or increment physical-voice generations.
        if (!native_storage_.build(
                native_capture,
                sample_rate,
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

    // Editable SNESAPU SRCE v2 path. Unlike the native libgme hook, this
    // producer already supplies host-rate source audio, exact per-sample
    // voice-local L/R coefficient trajectories, and the final shared wet L/R
    // contribution. The projection embeds exact route *magnitude* in each dry
    // mono lane, keeps signed route as presentation evidence, and marks the
    // arithmetic preapplied so Omniphony cannot double it.
    bool consume_snesapu_reference_window(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        const spc_runtime_spatial_capture_view& runtime_capture,
        const snesapu_source_transport_v2::view& source) noexcept
    {
        clear_errors();
        if (!validate_reference_window(reference_frame_start, reference_frame_count))
            return false;
        if (!source.valid() || source.frame_count() != reference_frame_count)
            return fail(spc_runtime_host_pipeline_error::snesapu_source_rejected);
        if (host_session_.session_epoch() >
            static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()))
            return fail(spc_runtime_host_pipeline_error::snesapu_source_rejected);

        // Validate the producer envelope before advancing persistent runtime
        // state. Per-plane finite checks happen transactionally per segment; a
        // later failure flushes the already-audible reference window.
        if (!build_spatial_window(
                runtime_capture,
                window_start_tick,
                tick_rate,
                sample_rate,
                reference_frame_count))
            return false;

        return push_snesapu_segments(
            reference_frame_start,
            reference_frame_count,
            source,
            static_cast<std::uint32_t>(host_session_.session_epoch()));
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
    snesapu_source_object_projection_error last_snesapu_projection_error() const noexcept {
        return last_snesapu_projection_error_;
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

        return validate_host_window_end(reference_frame_start, reference_frame_count);
    }

    bool push_snesapu_segments(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count,
        const snesapu_source_transport_v2::view& source,
        std::uint32_t echo_generation) noexcept
    {
        const auto* segments = spatial_adapter_.segments();
        const std::size_t segment_count = spatial_adapter_.segment_count();
        for (std::size_t index = 0; index < segment_count; ++index) {
            const auto& segment = segments[index];
            if (!snesapu_projection_.build(
                    source,
                    segment.reference_frame_offset,
                    segment.sources,
                    echo_generation)) {
                last_snesapu_projection_error_ = snesapu_projection_.last_error();
                return flush_after_host_failure(
                    reference_frame_start,
                    reference_frame_count,
                    spc_runtime_host_pipeline_error::snesapu_source_rejected);
            }

            const std::uint64_t segment_reference_frame =
                reference_frame_start + static_cast<std::uint64_t>(segment.reference_frame_offset);
            if (!host_session_.push_at(segment_reference_frame, snesapu_projection_.block())) {
                last_host_error_ = host_session_.last_error();
                return flush_after_host_failure(
                    reference_frame_start,
                    reference_frame_count,
                    spc_runtime_host_pipeline_error::host_session_rejected);
            }
        }

        return validate_host_window_end(reference_frame_start, reference_frame_count);
    }

    bool validate_host_window_end(
        std::uint64_t reference_frame_start,
        std::size_t reference_frame_count) noexcept
    {
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
        snesapu_projection_.reset();
        host_session_.reset(
            vgmtooling::model::spatial_source_host_discontinuity::decoder_flush,
            next_reference_frame);
        return fail(error);
    }

    void clear_errors() noexcept {
        last_error_ = spc_runtime_host_pipeline_error::none;
        last_native_error_ = spc_native_exact_source_error::none;
        last_snesapu_projection_error_ = snesapu_source_object_projection_error::none;
        last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
        last_host_error_ = vgmtooling::model::spatial_source_host_session_error::none;
    }

    bool fail(spc_runtime_host_pipeline_error error) noexcept {
        last_error_ = error;
        return false;
    }

    spatial_adapter_type spatial_adapter_{};
    native_storage_type native_storage_{};
    snesapu_projection_type snesapu_projection_{};
    host_session_type host_session_{};
    spc_runtime_host_pipeline_error last_error_ = spc_runtime_host_pipeline_error::none;
    spc_native_exact_source_error last_native_error_ = spc_native_exact_source_error::none;
    snesapu_source_object_projection_error last_snesapu_projection_error_ =
        snesapu_source_object_projection_error::none;
    spc_runtime_spatial_adapter_error last_spatial_error_ = spc_runtime_spatial_adapter_error::none;
    vgmtooling::model::spatial_source_host_session_error last_host_error_ =
        vgmtooling::model::spatial_source_host_session_error::none;
};

} // namespace gameaudio::spc
