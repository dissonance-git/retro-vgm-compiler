#pragma once

namespace gameaudio::vgm {

struct authored_stereo_route {
    float left = 0.0f;
    float right = 0.0f;
};

// Preserve YM2612's authored LR enable bits exactly for the first enhanced
// listening baseline. Spatial expansion belongs to a later source-domain stage;
// improved synthesis must prove itself before geometry changes at the same time.
constexpr authored_stereo_route ym2612_authored_route(bool left_enabled, bool right_enabled) noexcept {
    return authored_stereo_route{
        left_enabled ? 1.0f : 0.0f,
        right_enabled ? 1.0f : 0.0f,
    };
}

} // namespace gameaudio::vgm
