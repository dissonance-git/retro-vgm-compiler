#pragma once

#include "authored_stereo_route.h"
#include "../../../model/spatial_source.h"

#include <cstdint>

namespace gameaudio::vgm {

// Device tags share the high-byte namespace used by Genesis spatial source IDs.
// Genesis currently occupies 1..4, so YM2151/OPM uses 5. This keeps mixed-chip
// VGM source IDs distinct within one controlled playback generation.
enum class ym2151_spatial_device : std::uint8_t {
    fm = 5,
};

constexpr std::uint64_t ym2151_spatial_source_id(
    ym2151_spatial_device device,
    std::uint8_t instance,
    std::uint8_t channel,
    std::uint32_t episode_generation) noexcept
{
    return (static_cast<std::uint64_t>(device) << 56u)
        | (static_cast<std::uint64_t>(instance) << 48u)
        | (static_cast<std::uint64_t>(channel) << 40u)
        | static_cast<std::uint64_t>(episode_generation);
}

constexpr vgmtooling::model::spatial_source_evidence make_ym2151_spatial_source(
    std::uint8_t instance,
    std::uint8_t channel,
    std::uint32_t episode_generation,
    authored_stereo_route route) noexcept
{
    vgmtooling::model::spatial_source_evidence source;
    source.source_id = ym2151_spatial_source_id(
        ym2151_spatial_device::fm, instance, channel, episode_generation);
    source.generation = episode_generation;
    source.family = vgmtooling::model::spatial_source_family::vgm;
    source.physical_slot_present = true;
    source.physical_slot = channel;
    source.stereo_route.present = true;
    source.stereo_route.left_gain = vgmtooling::model::clamp_unit_gain(route.left);
    source.stereo_route.right_gain = vgmtooling::model::clamp_unit_gain(route.right);
    source.stereo_route.authority =
        vgmtooling::model::spatial_evidence_authority::device_authored;
    return source;
}

} // namespace gameaudio::vgm
