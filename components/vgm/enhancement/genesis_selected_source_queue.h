#pragma once

#include "genesis_enhanced_recomposition.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct genesis_selected_source_sample {
    double left = 0.0;
    double right = 0.0;
    bool present = false;
    bool exact = false;
};

struct genesis_selected_source_frame {
    std::uint64_t ordinal = 0;
    std::array<genesis_selected_source_sample, genesis_recomposition_source_count> source{};
};

// Bounded ordinal bridge between PlayerA engine/render-ahead time and the host
// delivery clock. Every entry starts from exact reference source contributions;
// independently admitted FM/PSG/DAC descendants replace only their own lanes.
// The final source frame is popped by the same output ordinal as the protected
// whole-frame transport, so Spatial never sees a source from a future engine
// frame or has to recover identities from the final stereo mix.
template <std::size_t Capacity>
class genesis_selected_source_queue {
    static_assert(Capacity > 0, "selected source queue capacity must be non-zero");

public:
    static constexpr std::size_t source_count = genesis_recomposition_source_count;

    void reset(std::uint64_t next_ordinal = 0) noexcept {
        head_ = 0;
        size_ = 0;
        next_ordinal_ = next_ordinal;
        valid_ = true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t size() const noexcept { return size_; }
    std::uint64_t next_ordinal() const noexcept { return next_ordinal_; }

    bool push_reference(const genesis_selected_source_frame& frame) noexcept {
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

    // Family-local replacement. Invalid replacement audio is rejected without
    // mutating the queued reference lane, allowing the caller to keep that
    // family on protected/reference source quality. An ordinal miss is different:
    // it means source time and audible frame time diverged, so the queue fails.
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

    // Explicit producer provenance form used by generated host glue. A source
    // family may only replace an exact isolated lane. Refusing `exact=false`
    // without mutating queue state makes that assumption executable rather than
    // a comment at the caller.
    bool replace_source(
        std::uint64_t ordinal,
        std::size_t source_index,
        double left,
        double right,
        bool exact) noexcept
    {
        return exact && replace_source(ordinal, source_index, left, right);
    }

    bool pop_expected(
        std::uint64_t ordinal,
        genesis_selected_source_frame& output) noexcept
    {
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
    static bool frame_valid(const genesis_selected_source_frame& frame) noexcept {
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
        const std::uint64_t first_ordinal = next_ordinal_ - static_cast<std::uint64_t>(size_);
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

    std::array<genesis_selected_source_frame, Capacity> frames_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::uint64_t next_ordinal_ = 0;
    bool valid_ = true;
};

} // namespace gameaudio::vgm
