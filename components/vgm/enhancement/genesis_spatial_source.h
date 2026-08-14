#pragma once

#include "authored_stereo_route.h"
#include "../../../model/spatial_source.h"

#include <cstdint>

namespace gameaudio::vgm {

enum class genesis_spatial_device : std::uint8_t {
    ym2612_fm = 1,
    ym2612_dac = 2,
    sn76489_tone = 3,
    sn76489_noise = 4,
};

// Stable within one controlled playback generation. This is an implementation
// source id, not a persistent musical-part identity.
constexpr std::uint64_t genesis_spatial_source_id(
    genesis_spatial_device device,
    std::uint8_t instance,
    std::uint8_t slot,
    std::uint32_t episode_generation) noexcept {
    return (static_cast<std::uint64_t>(device) << 56u)
        | (static_cast<std::uint64_t>(instance) << 48u)
        | (static_cast<std::uint64_t>(slot) << 40u)
        | static_cast<std::uint64_t>(episode_generation);
}

constexpr vgmtooling::model::spatial_source_evidence make_genesis_spatial_source(
    genesis_spatial_device device,
    std::uint8_t instance,
    std::uint8_t slot,
    std::uint32_t episode_generation,
    authored_stereo_route route) noexcept {
    vgmtooling::model::spatial_source_evidence source;
    source.source_id = genesis_spatial_source_id(device, instance, slot, episode_generation);
    source.generation = episode_generation;
    source.family = vgmtooling::model::spatial_source_family::vgm;
    source.physical_slot_present = true;
    source.physical_slot = slot;
    source.stereo_route.present = true;
    source.stereo_route.left_gain = vgmtooling::model::clamp_unit_gain(route.left);
    source.stereo_route.right_gain = vgmtooling::model::clamp_unit_gain(route.right);
    source.stereo_route.authority = vgmtooling::model::spatial_evidence_authority::device_authored;
    return source;
}

} // namespace gameaudio::vgm
