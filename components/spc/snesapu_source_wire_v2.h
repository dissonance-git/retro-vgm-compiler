#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Process-boundary ABI for the editable SNESAPU causal-source sidecar.
// Deliberately contains no musical/spatial model types so the 32-bit child and
// 64-bit foobar parent can share framing constants without presentation code.
struct snesapu_source_wire_v2 {
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

    static_assert(sizeof(header) == 24, "SNESAPU SRCE v2 wire header ABI changed");

    static constexpr std::uint32_t stream_block_samples(
        std::uint32_t sample_rate,
        bool source_capture_enabled) noexcept
    {
        std::uint32_t frames = sample_rate / 100u; // historical 10 ms child block
        if (frames == 0u)
            frames = 1u;
        if (source_capture_enabled && frames > max_frames)
            frames = static_cast<std::uint32_t>(max_frames);
        return frames;
    }

    static constexpr bool is_audio_plane(std::size_t plane) noexcept {
        return plane < voice_count || plane == echo_left_plane || plane == echo_right_plane;
    }

    static constexpr bool header_valid(const header& value) noexcept {
        return value.magic == magic
            && value.version == version
            && value.header_size == sizeof(header)
            && value.block_samples > 0u
            && value.block_samples <= max_frames
            && value.plane_count == plane_count
            && value.sample_format == format_float32
            && value.audio_lanes == audio_lane_count
            && value.reserved16 == 0u
            && value.reserved32 == 0u;
    }
};

} // namespace gameaudio::spc
