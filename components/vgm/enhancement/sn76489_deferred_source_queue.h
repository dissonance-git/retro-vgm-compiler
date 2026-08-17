#pragma once

#include "sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct sn76489_deferred_source_frame {
    std::uint64_t ordinal = 0;
    std::int64_t left = 0;
    std::int64_t right = 0;
};

// Bounded realtime bridge for PlayerA-style render-ahead.
//
// The ordinary Enhanced PSG block path is intentionally decode-block local.
// A deferred renderer needs a different clock: command events and generated
// source frames must advance on the player's absolute engine sample ordinal,
// even when fewer samples have been delivered to the host. This queue owns only
// that timing translation. The SN76489 synthesis law remains sn76489_enhanced.
//
// Call render_until() immediately before applying each command at its absolute
// sample ordinal, then again at the end of each engine-render span. The stereo
// mask is constant inside every such command-free interval, so source routing is
// exact without recording a second event stream.
template <std::size_t Capacity, std::size_t ScratchFrames = 256>
class sn76489_deferred_source_queue {
    static_assert(Capacity > 0, "deferred PSG queue capacity must be non-zero");
    static_assert(ScratchFrames > 0, "deferred PSG scratch size must be non-zero");

public:
    static constexpr std::size_t stem_count = sn76489_enhanced::stem_count;
    static constexpr double mame_native_channel_full_scale = 4096.0;

    void reset(std::uint64_t next_ordinal = 0) noexcept {
        head_ = 0;
        size_ = 0;
        next_ordinal_ = next_ordinal;
        valid_ = true;
    }

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::uint64_t next_ordinal() const noexcept { return next_ordinal_; }

    // Generate every frame in [next_ordinal(), end_ordinal). Failure is
    // transactional with respect to queue capacity: capacity/range checks occur
    // before the synth advances, so the caller may fail closed without a partly
    // advanced descendant clock.
    bool render_until(
        sn76489_enhanced& synth,
        std::uint64_t end_ordinal,
        std::int16_t libvgm_volume_left,
        std::int16_t libvgm_volume_right) noexcept
    {
        if (!valid_ || !synth.supported() || end_ordinal < next_ordinal_)
            return fail();

        const std::uint64_t count64 = end_ordinal - next_ordinal_;
        if (count64 > static_cast<std::uint64_t>(Capacity - size_)
            || count64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            return fail();
        std::size_t remaining = static_cast<std::size_t>(count64);

        const double scale_left = mame_native_channel_full_scale
            * static_cast<double>(libvgm_volume_left);
        const double scale_right = mame_native_channel_full_scale
            * static_cast<double>(libvgm_volume_right);
        if (!std::isfinite(scale_left) || !std::isfinite(scale_right))
            return fail();

        while (remaining != 0) {
            const std::size_t chunk = remaining < ScratchFrames ? remaining : ScratchFrames;
            float* outputs[stem_count]{};
            for (std::size_t channel = 0; channel < stem_count; ++channel)
                outputs[channel] = scratch_[channel].data();

            const std::uint8_t stereo_mask = synth.stereo_mask();
            synth.render(outputs, chunk);

            for (std::size_t frame = 0; frame < chunk; ++frame) {
                std::int64_t left = 0;
                std::int64_t right = 0;
                for (std::size_t channel = 0; channel < stem_count; ++channel) {
                    const float mono = scratch_[channel][frame];
                    if (!std::isfinite(mono))
                        return fail();

                    if ((stereo_mask & static_cast<std::uint8_t>(0x10u << channel)) != 0u) {
                        const float scaled = static_cast<float>(
                            static_cast<double>(mono) * scale_left);
                        if (!accumulate_rounded(scaled, left))
                            return fail();
                    }
                    if ((stereo_mask & static_cast<std::uint8_t>(0x01u << channel)) != 0u) {
                        const float scaled = static_cast<float>(
                            static_cast<double>(mono) * scale_right);
                        if (!accumulate_rounded(scaled, right))
                            return fail();
                    }
                }

                const std::size_t index = (head_ + size_) % Capacity;
                frames_[index] = {next_ordinal_, left, right};
                ++size_;
                ++next_ordinal_;
            }
            remaining -= chunk;
        }
        return true;
    }

    // The consumer knows which absolute PlayerA frame it is composing. A queue
    // mismatch is therefore identity/timing corruption, not an empty poll.
    bool pop_expected(
        std::uint64_t expected_ordinal,
        sn76489_deferred_source_frame& output) noexcept
    {
        if (!valid_ || size_ == 0)
            return false;
        if (frames_[head_].ordinal != expected_ordinal)
            return fail();
        output = frames_[head_];
        head_ = (head_ + 1u) % Capacity;
        --size_;
        return true;
    }

    void fail_closed() noexcept {
        valid_ = false;
        head_ = 0;
        size_ = 0;
    }

private:
    static bool accumulate_rounded(float sample, std::int64_t& total) noexcept {
        if (!std::isfinite(sample))
            return false;
        const long double value = static_cast<long double>(sample);
        if (value < static_cast<long double>(std::numeric_limits<std::int64_t>::min())
            || value > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
            return false;
        const std::int64_t rounded = static_cast<std::int64_t>(std::llround(sample));
        if ((rounded > 0 && total > std::numeric_limits<std::int64_t>::max() - rounded)
            || (rounded < 0 && total < std::numeric_limits<std::int64_t>::min() - rounded))
            return false;
        total += rounded;
        return true;
    }

    bool fail() noexcept {
        fail_closed();
        return false;
    }

    std::array<sn76489_deferred_source_frame, Capacity> frames_{};
    std::array<std::array<float, ScratchFrames>, stem_count> scratch_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::uint64_t next_ordinal_ = 0;
    bool valid_ = true;
};

} // namespace gameaudio::vgm
