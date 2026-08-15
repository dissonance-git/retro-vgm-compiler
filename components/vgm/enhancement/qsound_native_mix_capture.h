#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct qsound_native_mix_frame {
    std::uint64_t native_sample = 0;

    // A frame can be structurally present on the coherent native timeline while
    // its mix-accounting terms are intentionally unavailable. QSound init and
    // filter-refresh states advance the device without producing a fresh normal
    // echo/FIR/delay accounting sample. Never reinterpret those ticks as fresh
    // environment evidence.
    bool accounting_valid = false;
    std::int32_t echo_input = 0;
    std::int16_t echo_output = 0;
    std::array<std::int32_t, 2> wet_post_delay{};
    std::array<std::int32_t, 2> dry_post_delay{};
    std::array<std::int16_t, 2> reference_output{};
};

// Captures exact native-rate accounting terms from the historical superctr
// QSound renderer. The wet/dry values are the two delayed branch contributions
// immediately before their final sum, round and output clamp. They are not
// independent source stems and they do not imply a separable environmental
// return.
//
// valid() describes the structural capture timeline. Per-frame
// accounting_valid describes whether that native tick produced fresh mix
// accounting. Keeping the two states separate preserves init/refresh timing
// without smuggling stale accounting forward.
class qsound_native_mix_capture {
public:
    static constexpr std::size_t capacity = 4096;

    void begin_block() noexcept {
        count_ = 0;
        sample_rate_ = 0;
        overflowed_ = false;
        invalid_ = false;
    }

    void observe(
        std::uint8_t chip_id,
        std::uint32_t sample_rate,
        const qsound_native_mix_frame* frame) noexcept
    {
        if (invalid_ || overflowed_)
            return;

        if (chip_id != 0 || sample_rate == 0 || frame == nullptr) {
            invalid_ = true;
            return;
        }

        if (sample_rate_ == 0)
            sample_rate_ = sample_rate;
        else if (sample_rate_ != sample_rate) {
            invalid_ = true;
            return;
        }

        if (count_ != 0 && frame->native_sample != frames_[count_ - 1].native_sample + 1) {
            invalid_ = true;
            return;
        }

        if (count_ >= capacity) {
            overflowed_ = true;
            invalid_ = true;
            return;
        }

        frames_[count_++] = *frame;
    }

    bool valid() const noexcept { return !invalid_ && !overflowed_; }
    bool overflowed() const noexcept { return overflowed_; }
    std::uint32_t native_sample_rate() const noexcept { return sample_rate_; }
    std::size_t count() const noexcept { return count_; }

    std::uint64_t first_native_sample() const noexcept {
        return count_ != 0 ? frames_[0].native_sample : 0;
    }

    const qsound_native_mix_frame* frames() const noexcept {
        return frames_.data();
    }

private:
    std::array<qsound_native_mix_frame, capacity> frames_{};
    std::size_t count_ = 0;
    std::uint32_t sample_rate_ = 0;
    bool overflowed_ = false;
    bool invalid_ = false;
};

} // namespace gameaudio::vgm
