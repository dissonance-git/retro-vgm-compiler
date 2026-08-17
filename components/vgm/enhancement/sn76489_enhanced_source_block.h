#pragma once

#include "psg_block_capture.h"
#include "sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class sn76489_enhanced_source_block_error : std::uint8_t {
    none = 0,
    unsupported_renderer,
    invalid_frames,
    capture_overflow,
    unsorted_write,
    nonfinite_sample,
};

// Renders the higher-quality PSG descendant into the same device-source domain
// that libvgm's MAME SN76496 contribution occupies after its normal device
// resampler volume. This is intentionally *before* PlayerA song/fade gain.
//
// MAME's SN76496 table gives each of four channels MAX_OUTPUT/4 = 8192 native
// units at attenuation 0, then its bipolar source path applies >>1, so one
// reference channel's full-scale source contribution is 4096. libvgm's
// RESMPL_STATE then multiplies that source by volumeL/volumeR. Keeping this
// coordinate explicit lets the enhanced source replace, rather than layer over,
// an exact reference contribution.
template <std::size_t MaxFrames = 8192>
class sn76489_enhanced_source_block_storage {
    static_assert(MaxFrames > 0, "MaxFrames must be non-zero");

public:
    static constexpr std::size_t stem_count = sn76489_enhanced::stem_count;
    static constexpr double mame_native_channel_full_scale = 4096.0;

    struct stereo_source_view {
        const float* left = nullptr;
        const float* right = nullptr;
    };

    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
        last_error_ = sn76489_enhanced_source_block_error::none;
    }

    bool render(
        sn76489_enhanced& synth,
        const sn76489_timed_write* writes,
        std::size_t write_count,
        bool capture_overflowed,
        std::size_t frames,
        std::int16_t libvgm_volume_left,
        std::int16_t libvgm_volume_right) noexcept
    {
        reset();
        if (!synth.supported())
            return fail(sn76489_enhanced_source_block_error::unsupported_renderer);
        if (frames == 0 || frames > MaxFrames || (write_count != 0 && writes == nullptr))
            return fail(sn76489_enhanced_source_block_error::invalid_frames);
        if (capture_overflowed)
            return fail(sn76489_enhanced_source_block_error::capture_overflow);

        std::size_t previous = 0;
        for (std::size_t index = 0; index < write_count; ++index) {
            if (writes[index].sample_offset < previous)
                return fail(sn76489_enhanced_source_block_error::unsorted_write);
            previous = writes[index].sample_offset;
        }

        frame_count_ = frames;
        for (auto& stem : mono_)
            stem.fill(0.0f);
        for (auto& side : left_)
            side.fill(0.0f);
        for (auto& side : right_)
            side.fill(0.0f);

        // Render in write-bounded segments rather than calling render_timed once,
        // because Game Gear/SMS stereo mask is routing state, not part of the
        // mono source waveform. The mask active during each segment is therefore
        // sampled before its audio is generated.
        std::size_t cursor = 0;
        for (std::size_t index = 0; index < write_count; ++index) {
            const auto& event = writes[index];
            const std::size_t event_offset = event.sample_offset > frames
                ? frames : event.sample_offset;
            if (event_offset > cursor) {
                if (!render_segment(
                        synth,
                        cursor,
                        event_offset - cursor,
                        libvgm_volume_left,
                        libvgm_volume_right))
                    return false;
                cursor = event_offset;
            }

            if (event.kind == sn76489_write_kind::stereo_mask)
                synth.write_stereo_mask(event.data);
            else
                synth.write(event.data);
        }

        if (cursor < frames && !render_segment(
                synth,
                cursor,
                frames - cursor,
                libvgm_volume_left,
                libvgm_volume_right))
            return false;

        valid_ = true;
        return true;
    }

    bool render(
        sn76489_enhanced& synth,
        const psg_block_capture& capture,
        std::size_t instance,
        std::size_t frames,
        std::int16_t libvgm_volume_left,
        std::int16_t libvgm_volume_right) noexcept
    {
        return render(
            synth,
            capture.writes(instance),
            capture.count(instance),
            capture.overflowed(instance),
            frames,
            libvgm_volume_left,
            libvgm_volume_right);
    }

    bool valid() const noexcept { return valid_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    sn76489_enhanced_source_block_error last_error() const noexcept { return last_error_; }

    stereo_source_view source(std::size_t channel) const noexcept {
        if (!valid_ || channel >= stem_count)
            return {};
        return {left_[channel].data(), right_[channel].data()};
    }

    const float* mono(std::size_t channel) const noexcept {
        return valid_ && channel < stem_count ? mono_[channel].data() : nullptr;
    }

private:
    bool render_segment(
        sn76489_enhanced& synth,
        std::size_t offset,
        std::size_t frames,
        std::int16_t volume_left,
        std::int16_t volume_right) noexcept
    {
        const std::uint8_t mask = synth.stereo_mask();
        float* outputs[stem_count]{};
        for (std::size_t channel = 0; channel < stem_count; ++channel)
            outputs[channel] = mono_[channel].data() + offset;
        synth.render(outputs, frames);

        const double scale_left = mame_native_channel_full_scale
            * static_cast<double>(volume_left);
        const double scale_right = mame_native_channel_full_scale
            * static_cast<double>(volume_right);

        for (std::size_t channel = 0; channel < stem_count; ++channel) {
            const bool route_left = (mask & static_cast<std::uint8_t>(0x10u << channel)) != 0;
            const bool route_right = (mask & static_cast<std::uint8_t>(0x01u << channel)) != 0;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                const float mono = mono_[channel][offset + frame];
                if (!std::isfinite(mono))
                    return fail(sn76489_enhanced_source_block_error::nonfinite_sample);
                const double left = route_left ? static_cast<double>(mono) * scale_left : 0.0;
                const double right = route_right ? static_cast<double>(mono) * scale_right : 0.0;
                if (!std::isfinite(left) || !std::isfinite(right))
                    return fail(sn76489_enhanced_source_block_error::nonfinite_sample);
                left_[channel][offset + frame] = static_cast<float>(left);
                right_[channel][offset + frame] = static_cast<float>(right);
            }
        }
        return true;
    }

    bool fail(sn76489_enhanced_source_block_error error) noexcept {
        valid_ = false;
        last_error_ = error;
        return false;
    }

    std::array<std::array<float, MaxFrames>, stem_count> mono_{};
    std::array<std::array<float, MaxFrames>, stem_count> left_{};
    std::array<std::array<float, MaxFrames>, stem_count> right_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    sn76489_enhanced_source_block_error last_error_ =
        sn76489_enhanced_source_block_error::none;
};

} // namespace gameaudio::vgm
