#pragma once

#include "snesapu_source_transport_v2.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

enum class snesapu_source_transport_v2_storage_error : std::uint8_t {
    none = 0,
    invalid_header,
    null_planar,
    nonfinite_value,
    invalid_slice,
};

// Allocation-free owner for one normalized planar SRCE v2 block. It is the
// common boundary between the 32-bit spcplayer wire packet and the current
// dependency-free source projection/Omniphony host.
template <std::size_t MaxFrames = snesapu_source_transport_v2::max_frames>
class snesapu_source_transport_v2_storage {
    static_assert(MaxFrames > 0, "SRCE v2 storage capacity must be non-zero");
    static_assert(MaxFrames <= snesapu_source_transport_v2::max_frames,
        "SRCE v2 storage cannot exceed producer MIX_SIZE");

public:
    using transport = snesapu_source_transport_v2;

    void reset() noexcept {
        metadata_ = {};
        valid_ = false;
        last_error_ = snesapu_source_transport_v2_storage_error::none;
    }

    bool load_planar(
        const transport::header& metadata,
        const float* planar,
        std::size_t planar_values) noexcept
    {
        reset();
        transport::view incoming{&metadata, planar};
        if (!incoming.valid() || incoming.frame_count() > MaxFrames)
            return fail(snesapu_source_transport_v2_storage_error::invalid_header);
        if (planar == nullptr)
            return fail(snesapu_source_transport_v2_storage_error::null_planar);
        const std::size_t frames = incoming.frame_count();
        const std::size_t required = frames * transport::plane_count;
        if (planar_values != required)
            return fail(snesapu_source_transport_v2_storage_error::invalid_header);

        for (std::size_t plane = 0; plane < transport::plane_count; ++plane) {
            const float* src = planar + plane * frames;
            float* dst = data_.data() + plane * MaxFrames;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                if (!std::isfinite(src[frame]))
                    return fail(snesapu_source_transport_v2_storage_error::nonfinite_value);
                dst[frame] = src[frame];
            }
        }

        metadata_ = metadata;
        valid_ = true;
        return true;
    }

    // SNESAPU's native capture buffer is sample-major and keeps audio near its
    // internal 16-bit scale. Convert it once at the 32-bit child boundary to the
    // normalized planar wire form used by the x64 parent and Omniphony. Gain
    // planes remain dimensionless and therefore are never audio-scaled.
    bool load_sample_major(const float* sample_major, std::size_t frames) noexcept {
        reset();
        if (sample_major == nullptr || frames == 0 || frames > MaxFrames)
            return fail(snesapu_source_transport_v2_storage_error::invalid_header);

        constexpr float audio_scale = 1.0f / 32768.0f;
        for (std::size_t plane = 0; plane < transport::plane_count; ++plane) {
            const bool audio = plane < transport::voice_count
                || plane == transport::echo_left_plane
                || plane == transport::echo_right_plane;
            const float scale = audio ? audio_scale : 1.0f;
            float* dst = data_.data() + plane * MaxFrames;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                const float raw = sample_major[frame * transport::plane_count + plane];
                if (!std::isfinite(raw))
                    return fail(snesapu_source_transport_v2_storage_error::nonfinite_value);
                const float value = raw * scale;
                if (!std::isfinite(value))
                    return fail(snesapu_source_transport_v2_storage_error::nonfinite_value);
                dst[frame] = value;
            }
        }

        metadata_.magic = transport::magic;
        metadata_.version = transport::version;
        metadata_.header_size = static_cast<std::uint16_t>(sizeof(transport::header));
        metadata_.block_samples = static_cast<std::uint32_t>(frames);
        metadata_.plane_count = static_cast<std::uint16_t>(transport::plane_count);
        metadata_.sample_format = transport::format_float32;
        metadata_.audio_lanes = static_cast<std::uint16_t>(transport::audio_lane_count);
        valid_ = true;
        return true;
    }

    template <std::size_t OtherMaxFrames>
    bool copy_slice_from(
        const snesapu_source_transport_v2_storage<OtherMaxFrames>& source,
        std::size_t source_offset,
        std::size_t destination_offset,
        std::size_t count,
        std::size_t destination_frames) noexcept
    {
        if (!source.valid() || destination_frames == 0 || destination_frames > MaxFrames
            || source_offset > source.frame_count()
            || count > source.frame_count() - source_offset
            || destination_offset > destination_frames
            || count > destination_frames - destination_offset)
            return fail(snesapu_source_transport_v2_storage_error::invalid_slice);

        if (!valid_) {
            metadata_ = source.metadata();
            metadata_.block_samples = static_cast<std::uint32_t>(destination_frames);
            valid_ = true;
        } else if (frame_count() != destination_frames) {
            return fail(snesapu_source_transport_v2_storage_error::invalid_slice);
        }

        const auto src_view = source.view();
        for (std::size_t plane = 0; plane < transport::plane_count; ++plane) {
            const float* src = src_view.plane(plane) + source_offset;
            float* dst = data_.data() + plane * MaxFrames + destination_offset;
            for (std::size_t frame = 0; frame < count; ++frame)
                dst[frame] = src[frame];
        }
        return true;
    }

    bool valid() const noexcept {
        return valid_ && view().valid() && frame_count() <= MaxFrames;
    }

    std::size_t frame_count() const noexcept {
        return valid_ ? static_cast<std::size_t>(metadata_.block_samples) : 0;
    }

    const transport::header& metadata() const noexcept { return metadata_; }

    // Expose a contiguous planar packet with a compact plane stride equal to the
    // actual frame count. The fixed internal stride is MaxFrames, so compacting
    // happens into wire_ only when a view is requested. This is bounded and
    // allocation-free.
    transport::view view() const noexcept {
        if (!valid_ || metadata_.block_samples == 0 || metadata_.block_samples > MaxFrames)
            return {};
        const std::size_t frames = static_cast<std::size_t>(metadata_.block_samples);
        for (std::size_t plane = 0; plane < transport::plane_count; ++plane) {
            const float* src = data_.data() + plane * MaxFrames;
            float* dst = wire_.data() + plane * frames;
            for (std::size_t frame = 0; frame < frames; ++frame)
                dst[frame] = src[frame];
        }
        return {&metadata_, wire_.data()};
    }

    const float* plane_storage(std::size_t plane) const noexcept {
        return valid_ && plane < transport::plane_count
            ? data_.data() + plane * MaxFrames : nullptr;
    }

    snesapu_source_transport_v2_storage_error last_error() const noexcept {
        return last_error_;
    }

private:
    bool fail(snesapu_source_transport_v2_storage_error error) noexcept {
        valid_ = false;
        metadata_ = {};
        last_error_ = error;
        return false;
    }

    transport::header metadata_{};
    std::array<float, transport::plane_count * MaxFrames> data_{};
    mutable std::array<float, transport::plane_count * MaxFrames> wire_{};
    bool valid_ = false;
    snesapu_source_transport_v2_storage_error last_error_ =
        snesapu_source_transport_v2_storage_error::none;
};

} // namespace gameaudio::spc
