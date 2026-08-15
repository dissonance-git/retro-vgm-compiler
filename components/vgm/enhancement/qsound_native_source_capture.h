#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

constexpr std::size_t qsound_native_source_count = 19;

struct qsound_native_source_frame {
    std::uint64_t native_sample = 0;
    std::array<std::int16_t, qsound_native_source_count> source{};
};

// Captures the native QSound frames that libvgm's resampler actually pulls
// during one destination/output block. These are not output-rate frames.
//
// The callback path is deliberately fixed-capacity and allocation-free. Any
// ambiguity that would break one coherent 19-lane timeline invalidates the
// block instead of silently repairing it.
class qsound_native_source_capture {
public:
    static constexpr std::size_t source_count = qsound_native_source_count;
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
        std::uint64_t native_sample,
        const std::int16_t* source,
        std::size_t count) noexcept
    {
        if (invalid_ || overflowed_)
            return;

        // The VGM QSound command path currently represents one QSound device.
        // Preserve the identifier in the observer API, but fail closed here if
        // a second instance ever appears before the format model earns it.
        if (chip_id != 0 || sample_rate == 0 || source == nullptr || count != source_count) {
            invalid_ = true;
            return;
        }

        if (sample_rate_ == 0)
            sample_rate_ = sample_rate;
        else if (sample_rate_ != sample_rate) {
            invalid_ = true;
            return;
        }

        if (count_ != 0 && native_sample != frames_[count_ - 1].native_sample + 1) {
            invalid_ = true;
            return;
        }

        if (count_ >= capacity) {
            overflowed_ = true;
            invalid_ = true;
            return;
        }

        qsound_native_source_frame& frame = frames_[count_++];
        frame.native_sample = native_sample;
        for (std::size_t i = 0; i < source_count; ++i)
            frame.source[i] = source[i];
    }

    bool valid() const noexcept { return !invalid_ && !overflowed_; }
    bool overflowed() const noexcept { return overflowed_; }
    std::uint32_t native_sample_rate() const noexcept { return sample_rate_; }
    std::size_t count() const noexcept { return count_; }

    std::uint64_t first_native_sample() const noexcept {
        return count_ != 0 ? frames_[0].native_sample : 0;
    }

    const qsound_native_source_frame* frames() const noexcept {
        return frames_.data();
    }

private:
    std::array<qsound_native_source_frame, capacity> frames_{};
    std::size_t count_ = 0;
    std::uint32_t sample_rate_ = 0;
    bool overflowed_ = false;
    bool invalid_ = false;
};

} // namespace gameaudio::vgm
