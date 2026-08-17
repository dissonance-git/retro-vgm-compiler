#pragma once

#include "source_family_recomposition.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct selected_source_sample {
    double left = 0.0;
    double right = 0.0;
    bool present = false;
    bool exact = false;
};

template <std::size_t SourceCount>
struct selected_source_frame {
    static_assert(SourceCount > 0, "selected source count must be non-zero");

    std::uint64_t ordinal = 0;
    std::array<selected_source_sample, SourceCount> source{};
};

// Chip-neutral bounded bridge between producer/render-ahead time and the host
// delivery clock. Each queued frame begins from exact reference source lanes;
// independently admitted descendants may replace only their own exact lanes.
// Presentation therefore sees the already-selected source identity at the same
// ordinal as the audible protected stereo frame and never decides source quality.
template <std::size_t SourceCount, std::size_t Capacity>
class selected_source_queue {
    static_assert(SourceCount > 0, "selected source count must be non-zero");
    static_assert(Capacity > 0, "selected source queue capacity must be non-zero");

public:
    static constexpr std::size_t source_count = SourceCount;
    using frame_type = selected_source_frame<SourceCount>;

    void reset(std::uint64_t next_ordinal = 0) noexcept {
        head_ = 0;
        size_ = 0;
        next_ordinal_ = next_ordinal;
        valid_ = true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t size() const noexcept { return size_; }
    std::uint64_t next_ordinal() const noexcept { return next_ordinal_; }

    bool push_reference(const frame_type& frame) noexcept {
        if (!valid_ || size_ >= Capacity || frame.ordinal != next_ordinal_)
            return fail_closed();
        if (!frame_valid(frame))
            return fail_closed();
        if (next_ordinal_ == std::numeric_limits<std::uint64_t>::max())
            return fail_closed();

        frames_[(head_ + size_) % Capacity] = frame;
        ++size_;
        ++next_ordinal_;
        return true;
    }

    // Invalid replacement audio is rejected without mutating the queued exact
    // reference lane. Ordinal divergence is different: source time and audible
    // delivery time can no longer be proven identical, so the queue fails closed.
    bool replace_source(
        std::uint64_t ordinal,
        std::size_t source_index,
        double left,
        double right) noexcept
    {
        if (!valid_ || source_index >= source_count)
            return false;
        if (!std::isfinite(left) || !std::isfinite(right))
            return false;

        std::size_t queue_index = 0;
        if (!index_for_pending_ordinal(ordinal, queue_index)) {
            fail_closed();
            return false;
        }

        auto& destination = frames_[queue_index].source[source_index];
        destination.left = left;
        destination.right = right;
        destination.present = true;
        destination.exact = true;
        return true;
    }

    // Explicit producer-provenance form. An inexact candidate cannot replace an
    // isolated exact source lane and therefore leaves the reference lane intact.
    bool replace_source(
        std::uint64_t ordinal,
        std::size_t source_index,
        double left,
        double right,
        bool exact) noexcept
    {
        return exact && replace_source(ordinal, source_index, left, right);
    }

    bool pop_expected(std::uint64_t ordinal, frame_type& output) noexcept {
        if (!valid_ || size_ == 0)
            return false;
        if (frames_[head_].ordinal != ordinal)
            return fail_closed();

        output = frames_[head_];
        head_ = (head_ + 1u) % Capacity;
        --size_;
        return true;
    }

    void fail_closed_state() noexcept { (void)fail_closed(); }

private:
    static bool frame_valid(const frame_type& frame) noexcept {
        bool any = false;
        for (const auto& source : frame.source) {
            if (!source.present)
                continue;
            any = true;
            if (!source.exact || !std::isfinite(source.left) || !std::isfinite(source.right))
                return false;
        }
        return any;
    }

    bool index_for_pending_ordinal(
        std::uint64_t ordinal,
        std::size_t& output) const noexcept
    {
        if (size_ == 0)
            return false;
        const std::uint64_t first_ordinal =
            next_ordinal_ - static_cast<std::uint64_t>(size_);
        if (ordinal < first_ordinal || ordinal >= next_ordinal_)
            return false;
        const std::uint64_t offset64 = ordinal - first_ordinal;
        if (offset64 >= static_cast<std::uint64_t>(size_))
            return false;
        output = (head_ + static_cast<std::size_t>(offset64)) % Capacity;
        return frames_[output].ordinal == ordinal;
    }

    bool fail_closed() noexcept {
        valid_ = false;
        head_ = 0;
        size_ = 0;
        return false;
    }

    std::array<frame_type, Capacity> frames_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::uint64_t next_ordinal_ = 0;
    bool valid_ = true;
};

enum class selected_source_block_error : std::uint8_t {
    none = 0,
    invalid_frames,
    source_queue_invalid,
    ordinal_mismatch,
    topology_changed,
    inexact_source,
    sample_out_of_range,
    no_sources,
};

// Fixed-capacity adapter from the ordinal queue to one delivered host block.
// Exact topology is stable over the whole block. Exact digital-zero lanes are
// elided only after topology validation because silence is not a presentation
// object and should not require route evidence. An all-silent block consumes its
// matching ordinals but does not invalidate an otherwise coherent queue.
template <std::size_t SourceCount, std::size_t MaxFrames = 8192>
class selected_source_block_storage {
    static_assert(SourceCount > 0, "selected source count must be non-zero");
    static_assert(MaxFrames > 0, "selected source block capacity must be non-zero");

public:
    static constexpr std::size_t source_count = SourceCount;
    using source_array = std::array<source_family_stereo_view, source_count>;
    using frame_type = selected_source_frame<SourceCount>;

    void reset() noexcept {
        sources_ = {};
        present_.fill(false);
        frame_count_ = 0;
        first_ordinal_ = 0;
        valid_ = false;
        last_error_ = selected_source_block_error::none;
    }

    template <std::size_t QueueCapacity>
    bool consume(
        selected_source_queue<SourceCount, QueueCapacity>& queue,
        std::uint64_t first_ordinal,
        std::size_t frame_count) noexcept
    {
        reset();
        if (frame_count == 0 || frame_count > MaxFrames)
            return fail(selected_source_block_error::invalid_frames, nullptr);
        if (!queue.valid())
            return fail(selected_source_block_error::source_queue_invalid, nullptr);
        if (frame_count > static_cast<std::size_t>(
                std::numeric_limits<std::uint64_t>::max() - first_ordinal))
            return fail(selected_source_block_error::invalid_frames, &queue);

        std::array<bool, source_count> topology{};
        bool topology_initialized = false;

        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            const std::uint64_t ordinal =
                first_ordinal + static_cast<std::uint64_t>(frame);
            frame_type selected{};
            if (!queue.pop_expected(ordinal, selected))
                return fail(selected_source_block_error::ordinal_mismatch, &queue);

            std::array<bool, source_count> frame_topology{};
            for (std::size_t source_index = 0; source_index < source_count; ++source_index) {
                const auto& source = selected.source[source_index];
                frame_topology[source_index] = source.present;
                if (!source.present)
                    continue;
                if (!source.exact)
                    return fail(selected_source_block_error::inexact_source, &queue);
                if (!store_sample(source_index, frame, source.left, source.right))
                    return fail(selected_source_block_error::sample_out_of_range, &queue);
            }

            if (!topology_initialized) {
                topology = frame_topology;
                topology_initialized = true;
            } else if (topology != frame_topology) {
                return fail(selected_source_block_error::topology_changed, &queue);
            }
        }

        bool any_signal = false;
        present_ = topology;
        for (std::size_t source_index = 0; source_index < source_count; ++source_index) {
            if (!present_[source_index])
                continue;
            if (!source_has_signal(source_index, frame_count)) {
                present_[source_index] = false;
                continue;
            }
            sources_[source_index] = {
                left_[source_index].data(),
                right_[source_index].data(),
                true,
            };
            any_signal = true;
        }

        if (!any_signal)
            return fail(selected_source_block_error::no_sources, nullptr);

        first_ordinal_ = first_ordinal;
        frame_count_ = frame_count;
        valid_ = true;
        last_error_ = selected_source_block_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::uint64_t first_ordinal() const noexcept { return valid_ ? first_ordinal_ : 0; }
    std::size_t frame_count() const noexcept { return valid_ ? frame_count_ : 0; }
    selected_source_block_error last_error() const noexcept { return last_error_; }
    const source_array& sources() const noexcept { return sources_; }
    bool source_present(std::size_t index) const noexcept {
        return valid_ && index < source_count && present_[index];
    }

private:
    static bool representable_float(double value) noexcept {
        if (!std::isfinite(value))
            return false;
        const double limit = static_cast<double>(std::numeric_limits<float>::max());
        return value >= -limit && value <= limit;
    }

    bool store_sample(
        std::size_t source_index,
        std::size_t frame,
        double left,
        double right) noexcept
    {
        if (!representable_float(left) || !representable_float(right))
            return false;
        const float left_value = static_cast<float>(left);
        const float right_value = static_cast<float>(right);
        if (!std::isfinite(left_value) || !std::isfinite(right_value))
            return false;
        left_[source_index][frame] = left_value;
        right_[source_index][frame] = right_value;
        return true;
    }

    bool source_has_signal(std::size_t source_index, std::size_t frame_count) const noexcept {
        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            if (left_[source_index][frame] != 0.0f || right_[source_index][frame] != 0.0f)
                return true;
        }
        return false;
    }

    template <std::size_t QueueCapacity>
    bool fail(
        selected_source_block_error error,
        selected_source_queue<SourceCount, QueueCapacity>* queue) noexcept
    {
        if (queue != nullptr)
            queue->fail_closed_state();
        clear_output(error);
        return false;
    }

    bool fail(selected_source_block_error error, std::nullptr_t) noexcept {
        clear_output(error);
        return false;
    }

    void clear_output(selected_source_block_error error) noexcept {
        sources_ = {};
        present_.fill(false);
        frame_count_ = 0;
        first_ordinal_ = 0;
        valid_ = false;
        last_error_ = error;
    }

    std::array<std::array<float, MaxFrames>, source_count> left_{};
    std::array<std::array<float, MaxFrames>, source_count> right_{};
    source_array sources_{};
    std::array<bool, source_count> present_{};
    std::uint64_t first_ordinal_ = 0;
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    selected_source_block_error last_error_ = selected_source_block_error::none;
};

} // namespace gameaudio::vgm
