#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct authored_stereo_route {
    float left = 0.0f;
    float right = 0.0f;
};

// Preserve YM2612's authored LR enable bits exactly for the source-domain
// presentation layer. A renderer may later expand the source into 3-D, but the
// historical routing constraint remains attached as device-authored evidence.
constexpr authored_stereo_route ym2612_authored_route(bool left_enabled, bool right_enabled) noexcept {
    return authored_stereo_route{
        left_enabled ? 1.0f : 0.0f,
        right_enabled ? 1.0f : 0.0f,
    };
}

// Game Gear stereo register layout used by VGM command 0x4F and the SN76489
// family core: bits 4..7 enable channels 0..3 on the left output, while bits
// 0..3 enable the same channels on the right output. This is routing evidence,
// not a conventional constant-power pan law.
constexpr authored_stereo_route sn76489_authored_route(
    std::uint8_t stereo_mask,
    std::size_t channel) noexcept {
    const std::size_t slot = channel & 0x03u;
    const bool left_enabled = (stereo_mask & static_cast<std::uint8_t>(1u << (slot + 4u))) != 0;
    const bool right_enabled = (stereo_mask & static_cast<std::uint8_t>(1u << slot)) != 0;
    return authored_stereo_route{
        left_enabled ? 1.0f : 0.0f,
        right_enabled ? 1.0f : 0.0f,
    };
}

} // namespace gameaudio::vgm
