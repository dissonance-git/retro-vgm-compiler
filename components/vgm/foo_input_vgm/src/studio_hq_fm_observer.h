#pragma once

#include "studio_alignment_queue.h"
#include "studio_source_resampler.h"
#include "studio_source_stream.h"
#include "studio_source_timeline.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

struct studio_hq_fm_observation {
    std::uint64_t native_base = 0;
    std::uint64_t destination_base = 0;
    std::size_t native_frames = 0;
    std::size_t destination_frames = 0;
    std::size_t startup_reference_frames = 0;
    std::size_t newly_ready_studio_frames = 0;
    std::size_t pending_future_frames = 0;
    bool valid = false;
};

struct studio_hq_fm_gain {
    std::int32_t left = 1;
    std::int32_t right = 1;
};

template <std::size_t LaneCount>
struct studio_hq_fm_ready_frame {
    std::uint64_t destination_ordinal = 0;
    std::array<studio_stereo_sample, LaneCount> lane{};
    bool valid = false;
};

template <std::size_t LaneCount, std::size_t NativeCapacity, std::size_t PendingCapacity>
class studio_hq_fm_observer {
    static_assert(LaneCount > 0, "Studio HQ FM observer needs at least one lane.");
    static_assert(NativeCapacity >= studio_source_resampler_kernel::tap_count,
        "Studio HQ FM native history is too small for one FIR window.");
    static_assert(PendingCapacity > 0, "Studio HQ FM pending queue must be non-zero.");

public:
    using ready_frame = studio_hq_fm_ready_frame<LaneCount>;

    bool configure(std::uint32_t source_rate_hz, std::uint32_t destination_rate_hz) noexcept {
        reset();
        source_rate_hz_ = source_rate_hz;
        destination_rate_hz_ = destination_rate_hz;
        configured_ = kernel_.configure(
            static_cast<double>(source_rate_hz),
            static_cast<double>(destination_rate_hz));
        return configured_;
    }

    void reset() noexcept {
        for (auto& stream : streams_)
            stream.reset();
        pending_.reset();
        ready_head_ = 0;
        ready_count_ = 0;
        native_next_ = 0;
        destination_next_ = 0;
        released_next_ = 0;
        first_studio_destination_ordinal_ = 0;
        started_studio_domain_ = false;
        invalid_ = false;
    }

    [[nodiscard]] bool configured() const noexcept { return configured_; }
    [[nodiscard]] bool valid() const noexcept {
        if (!configured_ || invalid_ || !pending_.valid())
            return false;
        for (const auto& stream : streams_) {
            if (!stream.valid())
                return false;
        }
        return true;
    }

    [[nodiscard]] std::uint64_t next_native_ordinal() const noexcept { return native_next_; }
    [[nodiscard]] std::uint64_t next_destination_ordinal() const noexcept {
        return destination_next_;
    }
    [[nodiscard]] std::uint64_t next_release_ordinal() const noexcept {
        return released_next_;
    }
    [[nodiscard]] bool studio_domain_started() const noexcept {
        return started_studio_domain_;
    }
    [[nodiscard]] std::uint64_t first_studio_destination_ordinal() const noexcept {
        return first_studio_destination_ordinal_;
    }
    [[nodiscard]] std::size_t pending_frames() const noexcept { return pending_.size(); }
    [[nodiscard]] std::size_t ready_frames() const noexcept { return ready_count_; }

    bool pop_ready_frame(ready_frame& out) noexcept {
        if (!valid() || ready_count_ == 0)
            return false;
        out = ready_[ready_head_];
        ready_head_ = (ready_head_ + 1u) % PendingCapacity;
        --ready_count_;
        return out.valid;
    }

    template <typename SourceSample>
    bool append_initial_pregeneration(
        const std::array<const SourceSample*, LaneCount>& lanes,
        std::size_t frame_count) noexcept {
        if (!valid() || destination_next_ != 0 || !pending_.empty() || ready_count_ != 0) {
            invalid_ = true;
            return false;
        }
        return append_native(lanes, frame_count);
    }

    template <typename SourceSample>
    studio_hq_fm_observation observe_segment(
        const studio_linear_timing_snapshot& timing,
        const std::array<const SourceSample*, LaneCount>& lanes,
        std::size_t native_frames,
        std::size_t destination_frames,
        studio_hq_fm_gain gain = {}) noexcept {
        studio_hq_fm_observation report;
        report.native_base = native_next_;
        report.destination_base = destination_next_;
        report.native_frames = native_frames;
        report.destination_frames = destination_frames;

        if (!valid() || !timing.valid()
            || timing.source_rate_hz != source_rate_hz_
            || timing.destination_rate_hz != destination_rate_hz_) {
            invalidate();
            return report;
        }

        if (!append_native(lanes, native_frames))
            return report;

        report.newly_ready_studio_frames += drain_ready();
        if (!valid())
            return report;

        for (std::size_t offset = 0; offset < destination_frames; ++offset) {
            if (destination_next_ == std::numeric_limits<std::uint64_t>::max()) {
                invalidate();
                return report;
            }
            const auto position = studio_linear_source_position(
                timing,
                report.native_base,
                static_cast<std::uint64_t>(offset));
            const auto window = plan_studio_source_window(position);
            if (!position.valid || !window.valid) {
                invalidate();
                return report;
            }

            if (window.first < 0) {
                if (started_studio_domain_ || !pending_.empty()) {
                    invalidate();
                    return report;
                }
                ++report.startup_reference_frames;
                ++destination_next_;
                released_next_ = destination_next_;
                continue;
            }

            if (!started_studio_domain_) {
                first_studio_destination_ordinal_ = destination_next_;
                started_studio_domain_ = true;
            }
            if (!pending_.push(destination_next_, position, gain)) {
                invalidate();
                return report;
            }
            ++destination_next_;
        }

        report.newly_ready_studio_frames += drain_ready();
        trim_history();
        report.pending_future_frames = pending_.size();
        report.valid = valid();
        return report;
    }

    std::size_t finish_reference_tail() noexcept {
        if (!valid())
            return 0;
        std::size_t released = 0;
        typename pending_queue::entry entry{};
        while (pending_.pop_reference_tail(entry)) {
            if (entry.destination_ordinal != released_next_) {
                invalidate();
                return 0;
            }
            ++released_next_;
            ++released;
        }
        trim_history();
        return released;
    }

private:
    using native_stream = studio_source_stream<NativeCapacity>;
    using pending_queue = studio_alignment_queue<studio_hq_fm_gain, PendingCapacity>;

    void invalidate() noexcept { invalid_ = true; }

    template <typename SourceSample>
    bool append_native(
        const std::array<const SourceSample*, LaneCount>& lanes,
        std::size_t frame_count) noexcept {
        if (frame_count != 0) {
            for (const auto* lane : lanes) {
                if (lane == nullptr) {
                    invalidate();
                    return false;
                }
            }
        }
        if (native_next_ > std::numeric_limits<std::uint64_t>::max() - frame_count) {
            invalidate();
            return false;
        }

        const std::uint64_t base = native_next_;
        for (std::size_t lane = 0; lane < LaneCount; ++lane) {
            if (!streams_[lane].append_converted(base, lanes[lane], frame_count)) {
                invalidate();
                return false;
            }
        }
        native_next_ += static_cast<std::uint64_t>(frame_count);
        return true;
    }

    [[nodiscard]] bool all_lanes_ready(studio_source_phase_position position) const noexcept {
        const auto window = plan_studio_source_window(position);
        if (!window.valid)
            return false;
        for (const auto& stream : streams_) {
            if (!stream.contains(window))
                return false;
        }
        return true;
    }

    std::size_t drain_ready() noexcept {
        std::size_t released = 0;
        while (const auto* front = pending_.front()) {
            if (!all_lanes_ready(front->source_position))
                break;
            if (ready_count_ == PendingCapacity) {
                invalidate();
                return 0;
            }

            ready_frame candidate{};
            candidate.destination_ordinal = front->destination_ordinal;
            for (std::size_t lane = 0; lane < LaneCount; ++lane) {
                const auto reconstructed = streams_[lane].reconstruct(
                    kernel_, front->source_position);
                if (!reconstructed.valid) {
                    invalidate();
                    return 0;
                }
                const double left = reconstructed.sample.left
                    * static_cast<double>(front->payload.left);
                const double right = reconstructed.sample.right
                    * static_cast<double>(front->payload.right);
                if (!std::isfinite(left) || !std::isfinite(right)) {
                    invalidate();
                    return 0;
                }
                candidate.lane[lane] = {left, right};
            }
            candidate.valid = true;

            typename pending_queue::entry entry{};
            if (!pending_.pop_ready(streams_[0], entry)) {
                invalidate();
                return 0;
            }
            if (entry.destination_ordinal != released_next_
                || entry.destination_ordinal != candidate.destination_ordinal) {
                invalidate();
                return 0;
            }

            ready_[(ready_head_ + ready_count_) % PendingCapacity] = candidate;
            ++ready_count_;
            ++released_next_;
            ++released;
        }
        return released;
    }

    void trim_history() noexcept {
        if (!valid())
            return;

        std::uint64_t keep_from = 0;
        if (const auto* front = pending_.front()) {
            const auto window = plan_studio_source_window(front->source_position);
            if (!window.valid || window.first < 0) {
                invalidate();
                return;
            }
            keep_from = static_cast<std::uint64_t>(window.first);
        } else {
            constexpr std::uint64_t reserve =
                static_cast<std::uint64_t>(studio_source_resampler_kernel::tap_count * 2u);
            keep_from = native_next_ > reserve ? native_next_ - reserve : 0;
        }

        for (auto& stream : streams_) {
            if (!stream.discard_before(keep_from)) {
                invalidate();
                return;
            }
        }
    }

    studio_source_resampler_kernel kernel_{};
    std::array<native_stream, LaneCount> streams_{};
    pending_queue pending_{};
    std::array<ready_frame, PendingCapacity> ready_{};
    std::size_t ready_head_ = 0;
    std::size_t ready_count_ = 0;
    std::uint32_t source_rate_hz_ = 0;
    std::uint32_t destination_rate_hz_ = 0;
    std::uint64_t native_next_ = 0;
    std::uint64_t destination_next_ = 0;
    std::uint64_t released_next_ = 0;
    std::uint64_t first_studio_destination_ordinal_ = 0;
    bool configured_ = false;
    bool started_studio_domain_ = false;
    bool invalid_ = false;
};

} // namespace foobar_vgm::source_audio
