#pragma once

#include "spc_source_bus.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Consolidated form of the mature SRCE v2 transport developed in the historical
// vgmspc foobar/SNESAPU path. It keeps audio truth and route-control truth
// separate instead of baking one block-level pan estimate into the source PCM.
//
// Plane layout:
//   0..7    dry voice audio after interpolation/noise + envelope, pre VxVOL
//   8..15   exact per-sample effective left coefficient for voices 0..7
//   16..23  exact per-sample effective right coefficient for voices 0..7
//   24      final shared wet contribution, left, after EVOL
//   25      final shared wet contribution, right, after EVOL
//
// The shared wet lanes are already route-gain-scaled. The dry lanes are not.
// Global master/fade arithmetic remains downstream of this transport.
struct snesapu_source_transport_v2 {
    static constexpr std::uint32_t magic = 0x45435253u; // "SRCE" little-endian
    static constexpr std::uint16_t version = 2u;
    static constexpr std::size_t voice_count = 8u;
    static constexpr std::size_t dry_base = 0u;
    static constexpr std::size_t gain_left_base = 8u;
    static constexpr std::size_t gain_right_base = 16u;
    static constexpr std::size_t echo_left_plane = 24u;
    static constexpr std::size_t echo_right_plane = 25u;
    static constexpr std::size_t audio_lane_count = 10u;
    static constexpr std::size_t plane_count = 26u;
    static constexpr std::size_t max_frames = 1024u;
    static constexpr std::uint16_t format_float32 = 1u;

    struct header {
        std::uint32_t magic = 0;
        std::uint16_t version = 0;
        std::uint16_t header_size = 0;
        std::uint32_t block_samples = 0;
        std::uint16_t plane_count = 0;
        std::uint16_t sample_format = 0;
        std::uint16_t audio_lanes = 0;
        std::uint16_t reserved16 = 0;
        std::uint32_t reserved32 = 0;
    };

    static_assert(sizeof(header) == 24, "SNESAPU SRCE v2 header ABI changed");

    struct view {
        const header* metadata = nullptr;
        const float* planar = nullptr;

        bool valid() const noexcept {
            return metadata != nullptr
                && planar != nullptr
                && metadata->magic == magic
                && metadata->version == version
                && metadata->header_size == sizeof(header)
                && metadata->block_samples > 0u
                && metadata->block_samples <= max_frames
                && metadata->plane_count == plane_count
                && metadata->sample_format == format_float32
                && metadata->audio_lanes == audio_lane_count
                && metadata->reserved16 == 0u
                && metadata->reserved32 == 0u;
        }

        std::size_t frame_count() const noexcept {
            return valid() ? static_cast<std::size_t>(metadata->block_samples) : 0u;
        }

        const float* plane(std::size_t index) const noexcept {
            if (!valid() || index >= plane_count)
                return nullptr;
            return planar + index * frame_count();
        }

        const float* dry_voice(std::size_t voice) const noexcept {
            return voice < voice_count ? plane(dry_base + voice) : nullptr;
        }

        const float* gain_left(std::size_t voice) const noexcept {
            return voice < voice_count ? plane(gain_left_base + voice) : nullptr;
        }

        const float* gain_right(std::size_t voice) const noexcept {
            return voice < voice_count ? plane(gain_right_base + voice) : nullptr;
        }

        const float* echo_left() const noexcept { return plane(echo_left_plane); }
        const float* echo_right() const noexcept { return plane(echo_right_plane); }

        spc_source_bus::frame_view source_bus_view() const noexcept {
            spc_source_bus::frame_view out;
            if (!valid())
                return out;
            for (std::size_t voice = 0; voice < voice_count; ++voice)
                out.dry_voice[voice] = dry_voice(voice);
            out.echo_left = echo_left();
            out.echo_right = echo_right();
            out.frame_count = frame_count();
            return out;
        }
    };
};

} // namespace gameaudio::spc
