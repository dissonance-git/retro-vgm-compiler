#pragma once

#include "spatial_source_host_assembler.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

enum class spatial_source_host_discontinuity : std::uint8_t {
    initialize = 0,
    seek,
    decoder_flush,
    track_change,
    decoder_reset,
    format_reconfigure,
};

enum class spatial_source_host_session_error : std::uint8_t {
    none = 0,
    inactive,
    noncontiguous_input,
    reference_timeline_overflow,
    assembler_rejected,
};

struct spatial_source_host_chunk {
    spatial_source_block_view sources{};
    std::uint64_t session_epoch = 0;
    std::uint64_t reference_frame_start = 0;
};

// Playback-lifetime fence around spatial_source_host_assembler. The assembler
// preserves source semantics across arbitrary host chunking; this wrapper ties
// those semantics to the protected reference playback timeline and guarantees
// that seeks/flushes cannot leak buffered evidence into a new playback epoch.
template <std::size_t MaxLanes = 64,
          std::size_t CapacityFrames = 4096,
          std::size_t MaxEvents = 512>
class spatial_source_host_session {
public:
    using assembler_type = spatial_source_host_assembler<MaxLanes, CapacityFrames, MaxEvents>;

    void reset(
        spatial_source_host_discontinuity reason,
        std::uint64_t next_reference_frame = 0) noexcept
    {
        discarded_frames_total_ += assembler_.buffered_frames();
        assembler_.reset();
        if (session_epoch_ != std::numeric_limits<std::uint64_t>::max())
            ++session_epoch_;
        else
            session_epoch_ = 1;
        active_ = true;
        last_discontinuity_ = reason;
        expected_push_frame_ = next_reference_frame;
        next_pull_frame_ = next_reference_frame;
        last_error_ = spatial_source_host_session_error::none;
        last_assembler_error_ = spatial_source_host_assembler_error::none;
    }

    void deactivate() noexcept {
        discarded_frames_total_ += assembler_.buffered_frames();
        assembler_.reset();
        active_ = false;
        expected_push_frame_ = 0;
        next_pull_frame_ = 0;
        last_error_ = spatial_source_host_session_error::none;
        last_assembler_error_ = spatial_source_host_assembler_error::none;
    }

    bool push_at(
        std::uint64_t reference_frame_start,
        const spatial_source_block_view& block) noexcept
    {
        last_error_ = spatial_source_host_session_error::none;
        last_assembler_error_ = spatial_source_host_assembler_error::none;
        if (!active_)
            return fail(spatial_source_host_session_error::inactive);
        if (reference_frame_start != expected_push_frame_)
            return fail(spatial_source_host_session_error::noncontiguous_input);
        if (block.frame_count > std::numeric_limits<std::uint64_t>::max() - reference_frame_start)
            return fail(spatial_source_host_session_error::reference_timeline_overflow);

        if (!assembler_.push(block)) {
            last_assembler_error_ = assembler_.last_error();
            return fail(spatial_source_host_session_error::assembler_rejected);
        }

        expected_push_frame_ = reference_frame_start + static_cast<std::uint64_t>(block.frame_count);
        return true;
    }

    spatial_source_host_chunk pull(std::size_t max_frames) noexcept {
        spatial_source_host_chunk result;
        if (!active_ || max_frames == 0)
            return result;

        result.sources = assembler_.pull(max_frames);
        if (result.sources.frame_count == 0)
            return {};

        result.session_epoch = session_epoch_;
        result.reference_frame_start = next_pull_frame_;
        next_pull_frame_ += static_cast<std::uint64_t>(result.sources.frame_count);
        return result;
    }

    bool active() const noexcept { return active_; }
    std::uint64_t session_epoch() const noexcept { return session_epoch_; }
    std::uint64_t expected_push_frame() const noexcept { return expected_push_frame_; }
    std::uint64_t next_pull_frame() const noexcept { return next_pull_frame_; }
    std::size_t buffered_frames() const noexcept { return assembler_.buffered_frames(); }
    std::uint64_t discarded_frames_total() const noexcept { return discarded_frames_total_; }
    spatial_source_host_discontinuity last_discontinuity() const noexcept { return last_discontinuity_; }
    spatial_source_host_session_error last_error() const noexcept { return last_error_; }
    spatial_source_host_assembler_error last_assembler_error() const noexcept {
        return last_assembler_error_;
    }
    bool last_pull_identity_limited() const noexcept {
        return assembler_.last_pull_identity_limited();
    }

private:
    bool fail(spatial_source_host_session_error error) noexcept {
        last_error_ = error;
        return false;
    }

    assembler_type assembler_{};
    bool active_ = false;
    std::uint64_t session_epoch_ = 0;
    std::uint64_t expected_push_frame_ = 0;
    std::uint64_t next_pull_frame_ = 0;
    std::uint64_t discarded_frames_total_ = 0;
    spatial_source_host_discontinuity last_discontinuity_ =
        spatial_source_host_discontinuity::initialize;
    spatial_source_host_session_error last_error_ = spatial_source_host_session_error::none;
    spatial_source_host_assembler_error last_assembler_error_ =
        spatial_source_host_assembler_error::none;
};

} // namespace vgmtooling::model
