#pragma once

#include "ym2612_pcm_stream_bank.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct ym2612_pcm_source_frame {
    std::uint64_t ordinal = 0;
    std::int64_t left = 0;
    std::int64_t right = 0;
    bool replace_reference = false;
};

// Bounded engine-clock bridge for source-bank YM2612 DAC playback.
//
// The DAC-control observer changes stream state at exact PlayerA sample
// ordinals. render_until() materializes the command-free interval preceding the
// next event, so ordinary decode blocks and deferred FM render-ahead consume the
// same source timeline. Frames carry an explicit ownership bit: no active stream
// leaves the protected DAC untouched, exactly one active stream may replace it,
// and ambiguous simultaneous ownership fails closed instead of guessing.
template <std::size_t Capacity>
class ym2612_pcm_source_queue {
    static_assert(Capacity > 0, "YM2612 PCM source queue capacity must be non-zero");

public:
    static constexpr double libvgm_dac_full_scale = 8448.0;

    void reset(std::uint64_t next_ordinal = 0) noexcept {
        head_ = 0;
        size_ = 0;
        next_ordinal_ = next_ordinal;
        valid_ = true;
    }

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::uint64_t next_ordinal() const noexcept { return next_ordinal_; }

    bool render_until(
        ym2612_pcm_stream_bank& bank,
        std::uint64_t end_ordinal,
        bool dac_enabled,
        bool pan_left,
        bool pan_right,
        std::int16_t libvgm_volume_left,
        std::int16_t libvgm_volume_right) noexcept
    {
        if (!valid_ || end_ordinal < next_ordinal_)
            return fail();

        const std::uint64_t count64 = end_ordinal - next_ordinal_;
        if (count64 > static_cast<std::uint64_t>(Capacity - size_)
            || count64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            return fail();

        auto candidate = bank;
        std::size_t staged = 0;
        std::uint64_t ordinal = next_ordinal_;
        const std::size_t count = static_cast<std::size_t>(count64);

        for (std::size_t frame = 0; frame < count; ++frame) {
            const std::size_t owners = candidate.active_target_count();
            if (owners > 1)
                return fail();

            float mono = 0.0f;
            if (owners == 1 && !candidate.render(&mono, 1))
                return fail();
            if (owners == 0 && !candidate.render(nullptr, 1))
                return fail();

            ym2612_pcm_source_frame output{};
            output.ordinal = ordinal;
            output.replace_reference = owners == 1 && dac_enabled;
            if (output.replace_reference) {
                if (!std::isfinite(mono))
                    return fail();
                const double left = pan_left
                    ? static_cast<double>(mono) * libvgm_dac_full_scale
                        * static_cast<double>(libvgm_volume_left)
                    : 0.0;
                const double right = pan_right
                    ? static_cast<double>(mono) * libvgm_dac_full_scale
                        * static_cast<double>(libvgm_volume_right)
                    : 0.0;
                if (!round_checked(left, output.left) || !round_checked(right, output.right))
                    return fail();
            }

            const std::size_t index = (head_ + size_ + staged) % Capacity;
            frames_[index] = output;
            ++staged;
            ++ordinal;
        }

        bank = candidate;
        size_ += staged;
        next_ordinal_ = ordinal;
        return true;
    }

    bool pop_expected(std::uint64_t expected_ordinal, ym2612_pcm_source_frame& output) noexcept {
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
    static bool round_checked(double sample, std::int64_t& output) noexcept {
        if (!std::isfinite(sample))
            return false;
        const long double value = static_cast<long double>(sample);
        if (value < static_cast<long double>(std::numeric_limits<std::int64_t>::min())
            || value > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
            return false;
        output = static_cast<std::int64_t>(std::llround(sample));
        return true;
    }

    bool fail() noexcept {
        fail_closed();
        return false;
    }

    std::array<ym2612_pcm_source_frame, Capacity> frames_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::uint64_t next_ordinal_ = 0;
    bool valid_ = true;
};

} // namespace gameaudio::vgm
