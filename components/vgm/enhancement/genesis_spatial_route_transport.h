#pragma once

#include "genesis_spatial_source.h"
#include "spatial_route_transport.h"
#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

using genesis_spatial_route_transport_error = spatial_route_transport_error;

// Genesis owns only the device semantics: exact reset state, YM2612 B4-B6
// routing and Game Gear PSG stereo-mask writes. Absolute-ordinal evidence
// delivery is delegated to the generic VGM route transport.
template <std::size_t QueueCapacity = 1024, std::size_t MaxBlockEvents = 256>
class genesis_spatial_route_transport {
public:
    static constexpr std::size_t source_count = genesis_recomposition_source_count;
    using delivery_type = spatial_route_delivery_transport<
        source_count, QueueCapacity, MaxBlockEvents>;
    using evidence_array = typename delivery_type::evidence_array;
    using presence_array = typename delivery_type::presence_array;
    using event_array = typename delivery_type::event_array;
    using delivered_block = typename delivery_type::delivered_block;

    void reset() noexcept {
        delivery_.reset();

        // Pinned Nuked OPN2 reset semantics: all six channel pan_l/pan_r values
        // are one. DAC shares channel 6's authored output route while retaining
        // its own source identity.
        constexpr auto ym_reset_route = ym2612_authored_route(true, true);
        for (std::size_t channel = 0; channel < 6u; ++channel) {
            const std::size_t source_index =
                static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1)
                + channel;
            (void)delivery_.seed(
                source_index,
                make_genesis_spatial_source(
                    genesis_spatial_device::ym2612_fm,
                    0,
                    static_cast<std::uint8_t>(channel),
                    1,
                    ym_reset_route));
        }
        (void)delivery_.seed(
            static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac),
            make_genesis_spatial_source(
                genesis_spatial_device::ym2612_dac,
                0,
                0,
                1,
                ym_reset_route));

        // Pinned libvgm SN76496 reset semantics: stereo_mask = 0xFF. For mono
        // Sega PSG the core emits both outputs, represented by the same route.
        constexpr std::uint8_t psg_reset_mask = 0xFFu;
        for (std::size_t channel = 0; channel < 4u; ++channel) {
            const std::size_t source_index =
                static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)
                + channel;
            const auto device = channel < 3u
                ? genesis_spatial_device::sn76489_tone
                : genesis_spatial_device::sn76489_noise;
            (void)delivery_.seed(
                source_index,
                make_genesis_spatial_source(
                    device,
                    0,
                    static_cast<std::uint8_t>(channel),
                    1,
                    sn76489_authored_route(psg_reset_mask, channel)));
        }
    }

    bool valid() const noexcept { return delivery_.valid(); }
    genesis_spatial_route_transport_error last_error() const noexcept {
        return delivery_.last_error();
    }

    // Seek/state replay may seed exact current device state without inventing
    // old route writes as future audible events.
    bool seed(
        std::size_t source_index,
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        return delivery_.seed(source_index, evidence);
    }

    // Observe one exact VGM command at its already-resolved output ordinal.
    // Non-route commands remain a no-op.
    bool observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (!valid())
            return false;
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
                if (!delivery_.publish(
                        absolute_sample,
                        static_cast<std::size_t>(source),
                        make_genesis_spatial_source(
                            device,
                            0,
                            static_cast<std::uint8_t>(channel),
                            1,
                            sn76489_authored_route(mask, channel))))
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
            const std::size_t fm_source = static_cast<std::size_t>(
                static_cast<std::uint8_t>(genesis_recomposition_source::ym2612_fm1)
                + static_cast<std::uint8_t>(channel));
            if (!delivery_.publish(
                    absolute_sample,
                    fm_source,
                    make_genesis_spatial_source(
                        genesis_spatial_device::ym2612_fm,
                        0,
                        static_cast<std::uint8_t>(channel),
                        1,
                        route)))
                return false;

            // YM2612 DAC occupies channel 6's authored route but stays a distinct
            // source identity in the selected-source transport.
            if (channel == 5u) {
                if (!delivery_.publish(
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
        }

        return true;
    }

    bool prepare_delivered_block(
        std::uint64_t block_start,
        std::size_t frame_count,
        const presence_array& present,
        delivered_block& output) noexcept
    {
        return delivery_.prepare_delivered_block(
            block_start, frame_count, present, output);
    }

private:
    delivery_type delivery_{};
};

} // namespace gameaudio::vgm
