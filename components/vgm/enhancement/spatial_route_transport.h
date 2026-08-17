#pragma once

#include "../../../model/spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct spatial_evidence_transition {
    std::uint64_t ordinal = 0;
    std::size_t source_index = 0;
    vgmtooling::model::spatial_source_evidence evidence{};
};

// Chip-neutral sparse sideband for authored source-route changes while a VGM
// engine may render ahead of the host delivery clock. Producer ordinals must be
// monotonic. A stale transition means evidence time and audible time diverged,
// so the queue fails closed rather than silently shifting the route later.
template <std::size_t SourceCount, std::size_t Capacity>
class spatial_evidence_queue {
    static_assert(SourceCount > 0, "spatial evidence source count must be non-zero");
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

    bool push(const spatial_evidence_transition& transition) noexcept {
        if (!valid_ || size_ >= Capacity)
            return fail_closed();
        if (transition.source_index >= SourceCount || !evidence_valid(transition.evidence))
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
        if (frame_count > static_cast<std::size_t>(
                std::numeric_limits<std::uint64_t>::max() - block_start))
            return fail_closed();

        const std::uint64_t block_end =
            block_start + static_cast<std::uint64_t>(frame_count);
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
            event.lane_index = next.source_index;
            event.evidence = next.evidence;
            head_ = (head_ + 1u) % Capacity;
            --size_;
        }
        return true;
    }

    void fail_closed_state() noexcept { (void)fail_closed(); }

private:
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

    std::array<spatial_evidence_transition, Capacity> entries_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    bool have_last_ordinal_ = false;
    std::uint64_t last_ordinal_ = 0;
    bool valid_ = true;
};

enum class spatial_route_transport_error : std::uint8_t {
    none = 0,
    queue_invalid,
    queue_overflow_or_order,
    invalid_block,
    missing_initial_route,
};

// Shared delivery-clock route state. Device clients own register semantics and
// call seed()/publish() only with exact authored evidence. This layer knows only
// source identity, output ordinal, block presence and the rule that presentation
// must fail closed when an audible source has no route evidence at block start.
template <std::size_t SourceCount,
          std::size_t QueueCapacity = 1024,
          std::size_t MaxBlockEvents = 256>
class spatial_route_delivery_transport {
    static_assert(SourceCount > 0, "spatial route source count must be non-zero");

public:
    static constexpr std::size_t source_count = SourceCount;
    using evidence_array =
        std::array<vgmtooling::model::spatial_source_evidence, source_count>;
    using presence_array = std::array<bool, source_count>;
    using event_array =
        std::array<vgmtooling::model::spatial_source_evidence_event, MaxBlockEvents>;

    struct delivered_block {
        evidence_array initial_evidence{};
        event_array events{};
        std::size_t event_count = 0;
        bool routes_complete = false;
    };

    void reset() noexcept {
        queue_.reset();
        producer_evidence_ = {};
        delivered_evidence_ = {};
        producer_known_.fill(false);
        delivered_known_.fill(false);
        valid_ = true;
        last_error_ = spatial_route_transport_error::none;
    }

    bool valid() const noexcept { return valid_ && queue_.valid(); }
    spatial_route_transport_error last_error() const noexcept { return last_error_; }

    // Seed proven state at a seek/reset boundary without inventing a historical
    // output ordinal. Device clients should use this only when their pinned
    // renderer gives exact state for the destination.
    bool seed(
        std::size_t source_index,
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        if (!valid() || source_index >= source_count || !evidence_valid(evidence))
            return false;
        producer_evidence_[source_index] = evidence;
        delivered_evidence_[source_index] = evidence;
        producer_known_[source_index] = true;
        delivered_known_[source_index] = true;
        return true;
    }

    // Publish one exact route transition at its already-resolved output ordinal.
    bool publish(
        std::uint64_t ordinal,
        std::size_t source_index,
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        if (!valid())
            return fail(spatial_route_transport_error::queue_invalid);
        if (source_index >= source_count || !evidence_valid(evidence))
            return fail(spatial_route_transport_error::queue_overflow_or_order);

        producer_evidence_[source_index] = evidence;
        producer_known_[source_index] = true;
        if (!queue_.push({ordinal, source_index, evidence}))
            return fail(spatial_route_transport_error::queue_overflow_or_order);
        return true;
    }

    // Advance route evidence with the delivered audio clock. Events for absent
    // sources still update future delivered state, but are not forwarded as
    // presentation events for the current block.
    bool prepare_delivered_block(
        std::uint64_t block_start,
        std::size_t frame_count,
        const presence_array& present,
        delivered_block& output) noexcept
    {
        output = {};
        if (!valid())
            return fail(spatial_route_transport_error::queue_invalid);
        if (frame_count == 0)
            return fail(spatial_route_transport_error::invalid_block);

        event_array drained{};
        std::size_t drained_count = 0;
        if (!queue_.drain_block(block_start, frame_count, drained, drained_count))
            return fail(spatial_route_transport_error::queue_invalid);

        output.initial_evidence = delivered_evidence_;
        auto initial_known = delivered_known_;

        // Same-offset route writes define the state used by the first audible
        // sample, so fold them into initial evidence rather than emitting event 0.
        for (std::size_t index = 0; index < drained_count; ++index) {
            const auto& event = drained[index];
            if (event.frame_offset != 0u)
                continue;
            output.initial_evidence[event.lane_index] = event.evidence;
            initial_known[event.lane_index] = true;
        }

        for (std::size_t source = 0; source < source_count; ++source) {
            if (present[source] && !initial_known[source]) {
                output.routes_complete = false;
                last_error_ = spatial_route_transport_error::missing_initial_route;
                advance_delivered_state(drained, drained_count);
                return true;
            }
        }

        for (std::size_t index = 0; index < drained_count; ++index) {
            const auto& event = drained[index];
            if (event.frame_offset == 0u || !present[event.lane_index])
                continue;
            if (output.event_count >= output.events.size())
                return fail(spatial_route_transport_error::queue_overflow_or_order);
            output.events[output.event_count++] = event;
        }

        output.routes_complete = true;
        advance_delivered_state(drained, drained_count);
        last_error_ = spatial_route_transport_error::none;
        return true;
    }

private:
    static bool evidence_valid(
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        return evidence.family == vgmtooling::model::spatial_source_family::vgm
            && evidence.source_id != 0
            && evidence.stereo_route.present;
    }

    void advance_delivered_state(const event_array& events, std::size_t count) noexcept {
        for (std::size_t index = 0; index < count; ++index) {
            const auto& event = events[index];
            delivered_evidence_[event.lane_index] = event.evidence;
            delivered_known_[event.lane_index] = true;
        }
    }

    bool fail(spatial_route_transport_error error) noexcept {
        valid_ = false;
        queue_.fail_closed_state();
        last_error_ = error;
        return false;
    }

    spatial_evidence_queue<source_count, QueueCapacity> queue_{};
    evidence_array producer_evidence_{};
    evidence_array delivered_evidence_{};
    presence_array producer_known_{};
    presence_array delivered_known_{};
    bool valid_ = true;
    spatial_route_transport_error last_error_ = spatial_route_transport_error::none;
};

} // namespace gameaudio::vgm
