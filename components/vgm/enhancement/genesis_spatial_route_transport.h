#pragma once

#include "genesis_spatial_evidence_queue.h"
#include "genesis_spatial_source.h"
#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class genesis_spatial_route_transport_error : std::uint8_t {
    none = 0,
    queue_invalid,
    queue_overflow_or_order,
    invalid_block,
    missing_initial_route,
};

// Exact command->delivered-block transport for Genesis native stereo routing.
//
// The producer side observes only route-defining VGM commands and records them
// at their absolute output-sample ordinal. The consumer side advances on the
// foobar delivery clock. This matters when PlayerA renders ahead: a YM2612 B4-B6
// write or Game Gear PSG stereo-mask write must reach Omniphony in the block the
// listener actually hears, not the block in which the engine happened to run.
//
// Reset defaults are deliberately *not* guessed. A source is spatial-renderable
// only after an exact route command has established its route. A frame-0 route
// write is promoted into the block's initial evidence, so normal track
// initialization does not lose a whole block merely because reset state was
// unknown. Later first-known routes cannot retroactively define earlier frames;
// that block therefore stays on ordinary stereo while delivery state still
// advances for the next block.
template <std::size_t QueueCapacity = 1024, std::size_t MaxBlockEvents = 256>
class genesis_spatial_route_transport {
public:
    static constexpr std::size_t source_count = genesis_recomposition_source_count;
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
        last_error_ = genesis_spatial_route_transport_error::none;
    }

    bool valid() const noexcept { return valid_ && queue_.valid(); }
    genesis_spatial_route_transport_error last_error() const noexcept { return last_error_; }

    // Observe one exact VGM command at its already-resolved output ordinal.
    // Non-route commands are a no-op and remain valid.
    bool observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (!valid())
            return fail(genesis_spatial_route_transport_error::queue_invalid);
        if (event.kind != command_event_kind::command || event.payload == nullptr)
            return true;

        if (event.command == 0x4Fu && event.payload_size >= 1u) {
            const std::uint8_t mask = event.payload[0];
            for (std::size_t channel = 0; channel < 4u; ++channel) {
                const auto source = static_cast<genesis_recomposition_source>(
                    static_cast<std::uint8_t>(genesis_recomposition_source::sn76489_tone0)
                    + static_cast<std::uint8_t>(channel));
                const auto device = channel < 3u
                    ? genesis_spatial_device::sn76489_tone
                    : genesis_spatial_device::sn76489_noise;
                const auto evidence = make_genesis_spatial_source(
                    device,
                    0,
                    static_cast<std::uint8_t>(channel),
                    1,
                    sn76489_authored_route(mask, channel));
                if (!publish(absolute_sample, static_cast<std::size_t>(source), evidence))
                    return false;
            }
            return true;
        }

        // Primary YM2612 only. 0x52 is port 0 (channels 1-3), 0x53 is port 1
        // (channels 4-6). B4-B6 carry L/R enable in bits 7/6.
        if ((event.command == 0x52u || event.command == 0x53u)
            && event.payload_size >= 2u
            && event.payload[0] >= 0xB4u
            && event.payload[0] <= 0xB6u)
        {
            const std::size_t port = event.command == 0x53u ? 1u : 0u;
            const std::size_t channel = port * 3u
                + static_cast<std::size_t>(event.payload[0] - 0xB4u);
            const bool left = (event.payload[1] & 0x80u) != 0u;
            const bool right = (event.payload[1] & 0x40u) != 0u;
            const auto route = ym2612_authored_route(left, right);
            const auto fm_source = static_cast<std::size_t>(
                static_cast<std::uint8_t>(genesis_recomposition_source::ym2612_fm1)
                + static_cast<std::uint8_t>(channel));
            if (!publish(
                    absolute_sample,
                    fm_source,
                    make_genesis_spatial_source(
                        genesis_spatial_device::ym2612_fm,
                        0,
                        static_cast<std::uint8_t>(channel),
                        1,
                        route)))
                return false;

            // YM2612 DAC occupies channel 6's authored output route. Keep a
            // distinct source identity while sharing that exact device route.
            if (channel == 5u) {
                if (!publish(
                        absolute_sample,
                        static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac),
                        make_genesis_spatial_source(
                            genesis_spatial_device::ym2612_dac,
                            0,
                            0,
                            1,
                            route)))
                    return false;
            }
            return true;
        }

        return true;
    }

    // Advance route evidence with the delivered audio clock. `present` describes
    // the exact selected source topology for this block. Route events for absent
    // sources still update future delivered state but are not sent to Omniphony.
    bool prepare_delivered_block(
        std::uint64_t block_start,
        std::size_t frame_count,
        const presence_array& present,
        delivered_block& output) noexcept
    {
        output = {};
        if (!valid())
            return fail(genesis_spatial_route_transport_error::queue_invalid);
        if (frame_count == 0)
            return fail(genesis_spatial_route_transport_error::invalid_block);

        event_array drained{};
        std::size_t drained_count = 0;
        if (!queue_.drain_block(block_start, frame_count, drained, drained_count))
            return fail(genesis_spatial_route_transport_error::queue_invalid);

        output.initial_evidence = delivered_evidence_;
        auto initial_known = delivered_known_;

        // A route established at the first audible sample is valid initial
        // evidence. Apply all same-offset events in order; the last one is the
        // state actually used by that sample after command processing.
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
                last_error_ = genesis_spatial_route_transport_error::missing_initial_route;
                advance_delivered_state(drained, drained_count);
                return true;
            }
        }

        // Frame-0 events were folded into initial state above. Preserve every
        // later event in exact order, but only for lanes present in this block.
        for (std::size_t index = 0; index < drained_count; ++index) {
            const auto& event = drained[index];
            if (event.frame_offset == 0u || !present[event.lane_index])
                continue;
            if (output.event_count >= output.events.size())
                return fail(genesis_spatial_route_transport_error::queue_overflow_or_order);
            output.events[output.event_count++] = event;
        }

        output.routes_complete = true;
        advance_delivered_state(drained, drained_count);
        last_error_ = genesis_spatial_route_transport_error::none;
        return true;
    }

private:
    bool publish(
        std::uint64_t ordinal,
        std::size_t source_index,
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        if (source_index >= source_count)
            return fail(genesis_spatial_route_transport_error::queue_overflow_or_order);
        producer_evidence_[source_index] = evidence;
        producer_known_[source_index] = true;
        if (!queue_.push({ordinal, source_index, evidence}))
            return fail(genesis_spatial_route_transport_error::queue_overflow_or_order);
        return true;
    }

    void advance_delivered_state(const event_array& events, std::size_t count) noexcept {
        for (std::size_t index = 0; index < count; ++index) {
            const auto& event = events[index];
            delivered_evidence_[event.lane_index] = event.evidence;
            delivered_known_[event.lane_index] = true;
        }
    }

    bool fail(genesis_spatial_route_transport_error error) noexcept {
        valid_ = false;
        queue_.fail_closed_state();
        last_error_ = error;
        return false;
    }

    genesis_spatial_evidence_queue<QueueCapacity> queue_{};
    evidence_array producer_evidence_{};
    evidence_array delivered_evidence_{};
    presence_array producer_known_{};
    presence_array delivered_known_{};
    bool valid_ = true;
    genesis_spatial_route_transport_error last_error_ =
        genesis_spatial_route_transport_error::none;
};

} // namespace gameaudio::vgm
