#pragma once

#include "../../../model/spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct genesis_spatial_evidence_transition {
    std::uint64_t ordinal = 0;
    std::size_t source_index = 0;
    vgmtooling::model::spatial_source_evidence evidence{};
};

// Sparse realtime sideband for source-route changes that happen while PlayerA
// may be rendering ahead of the foobar delivery clock. Audio samples stay in
// genesis_selected_source_queue; this queue carries only evidence transitions.
//
// Producer ordinals must be monotonic. drain_block() consumes exactly the
// transitions belonging to one delivered half-open interval [start,end),
// converting absolute ordinals to frame offsets for Omniphony. A stale event
// means the source-evidence clock was not consumed with the audible clock and
// therefore fails closed instead of being silently shifted into a later block.
template <std::size_t Capacity>
class genesis_spatial_evidence_queue {
    static_assert(Capacity > 0, "spatial evidence queue capacity must be non-zero");

public:
    void reset() noexcept {
        head_ = 0;
        size_ = 0;
        have_last_ordinal_ = false;
        last_ordinal_ = 0;
        valid_ = true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t size() const noexcept { return size_; }

    bool push(const genesis_spatial_evidence_transition& transition) noexcept {
        if (!valid_ || size_ >= Capacity)
            return fail_closed();
        if (transition.source_index >= source_capacity
            || !evidence_valid(transition.evidence))
            return false;
        if (have_last_ordinal_ && transition.ordinal < last_ordinal_)
            return fail_closed();

        entries_[(head_ + size_) % Capacity] = transition;
        ++size_;
        last_ordinal_ = transition.ordinal;
        have_last_ordinal_ = true;
        return true;
    }

    template <std::size_t OutputCapacity>
    bool drain_block(
        std::uint64_t block_start,
        std::size_t frame_count,
        std::array<vgmtooling::model::spatial_source_evidence_event, OutputCapacity>& output,
        std::size_t& output_count) noexcept
    {
        output_count = 0;
        if (!valid_ || frame_count == 0)
            return false;
        if (frame_count > static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::max() - block_start))
            return fail_closed();
        const std::uint64_t block_end = block_start + static_cast<std::uint64_t>(frame_count);

        while (size_ != 0) {
            const auto& next = entries_[head_];
            if (next.ordinal < block_start)
                return fail_closed();
            if (next.ordinal >= block_end)
                break;
            if (output_count >= OutputCapacity)
                return fail_closed();

            auto& event = output[output_count++];
            event.frame_offset = static_cast<std::size_t>(next.ordinal - block_start);
            event.lane_index = next.source_index; // canonical index; bus remaps it.
            event.evidence = next.evidence;
            head_ = (head_ + 1u) % Capacity;
            --size_;
        }
        return true;
    }

    void fail_closed_state() noexcept { (void)fail_closed(); }

private:
    static constexpr std::size_t source_capacity = 11u;

    static bool evidence_valid(
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        return evidence.family == vgmtooling::model::spatial_source_family::vgm
            && evidence.source_id != 0
            && evidence.stereo_route.present;
    }

    bool fail_closed() noexcept {
        valid_ = false;
        head_ = 0;
        size_ = 0;
        have_last_ordinal_ = false;
        return false;
    }

    std::array<genesis_spatial_evidence_transition, Capacity> entries_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    bool have_last_ordinal_ = false;
    std::uint64_t last_ordinal_ = 0;
    bool valid_ = true;
};

} // namespace gameaudio::vgm
