#pragma once

#include "spatial_route_transport.h"
#include "vgm_command_event.h"
#include "yamaha_opm_register.h"
#include "ym2151_enhanced_recomposition.h"
#include "ym2151_spatial_source.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

using ym2151_spatial_route_transport_error = spatial_route_transport_error;

// YM2151/OPM device client for the generic delivered-route transport.
//
// Unlike the Genesis client, reset() intentionally does not seed a stereo route.
// We have exact evidence for authored OPM RL writes (registers 0x20..0x27), but
// no equally strong reason to present an unwritten reset value as authored route
// evidence. Until a route is written or explicitly seeded after state replay,
// source-aware Spatial therefore fails closed while ordinary stereo remains live.
template <std::size_t QueueCapacity = 1024, std::size_t MaxBlockEvents = 256>
class ym2151_spatial_route_transport {
public:
    static constexpr std::size_t source_count = ym2151_recomposition_source_count;
    using delivery_type = spatial_route_delivery_transport<
        source_count, QueueCapacity, MaxBlockEvents>;
    using evidence_array = typename delivery_type::evidence_array;
    using presence_array = typename delivery_type::presence_array;
    using event_array = typename delivery_type::event_array;
    using delivered_block = typename delivery_type::delivered_block;

    void reset() noexcept { delivery_.reset(); }

    bool valid() const noexcept { return delivery_.valid(); }
    ym2151_spatial_route_transport_error last_error() const noexcept {
        return delivery_.last_error();
    }

    bool seed(
        std::size_t source_index,
        const vgmtooling::model::spatial_source_evidence& evidence) noexcept
    {
        return delivery_.seed(source_index, evidence);
    }

    // Convenience for seek/state-replay boundaries after the exact current OPM
    // channel register state has already been reconstructed.
    bool seed_channel_route(
        std::size_t channel,
        std::uint8_t register_value,
        std::uint32_t episode_generation = 1u) noexcept
    {
        if (channel >= source_count)
            return false;
        const auto route = decode_opm_stereo_route(register_value);
        return delivery_.seed(
            channel,
            make_ym2151_spatial_source(
                0,
                static_cast<std::uint8_t>(channel),
                episode_generation,
                authored_stereo_route{
                    route.left ? 1.0f : 0.0f,
                    route.right ? 1.0f : 0.0f,
                }));
    }

    // VGM command 0x54 is primary YM2151 register write. Only RL/feedback/
    // algorithm registers 0x20..0x27 define the authored channel output route.
    // The second-chip mirror 0xA4 is intentionally outside this primary 8-lane
    // topology and therefore remains a no-op here.
    bool observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (!valid())
            return false;
        if (event.kind != command_event_kind::command || event.payload == nullptr)
            return true;
        if (event.command != 0x54u || event.payload_size < 2u)
            return true;
        if (!opm_algorithm_feedback_register(event.payload[0]))
            return true;

        const std::size_t channel = static_cast<std::size_t>(event.payload[0] & 0x07u);
        const auto route = decode_opm_stereo_route(event.payload[1]);
        return delivery_.publish(
            absolute_sample,
            channel,
            make_ym2151_spatial_source(
                0,
                static_cast<std::uint8_t>(channel),
                1,
                authored_stereo_route{
                    route.left ? 1.0f : 0.0f,
                    route.right ? 1.0f : 0.0f,
                }));
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
