#pragma once

#include "snesapu_source_wire_v2.h"
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
    using wire = snesapu_source_wire_v2;
    using header = wire::header;

    static constexpr std::uint32_t magic = wire::magic;
    static constexpr std::uint16_t version = wire::version;
    static constexpr std::size_t voice_count = wire::voice_count;
    static constexpr std::size_t dry_base = wire::dry_base;
    static constexpr std::size_t gain_left_base = wire::gain_left_base;
    static constexpr std::size_t gain_right_base = wire::gain_right_base;
    static constexpr std::size_t echo_left_plane = wire::echo_left_plane;
    static constexpr std::size_t echo_right_plane = wire::echo_right_plane;
    static constexpr std::size_t audio_lane_count = wire::audio_lane_count;
    static constexpr std::size_t plane_count = wire::plane_count;
    static constexpr std::size_t max_frames = wire::max_frames;
    static constexpr std::uint16_t format_float32 = wire::format_float32;

    struct view {
        const header* metadata = nullptr;
        const float* planar = nullptr;

        bool valid() const noexcept {
            return metadata != nullptr && planar != nullptr && wire::header_valid(*metadata);
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
