#pragma once

#include "qsound_native_source_capture.h"
#include "qsound_native_time_map.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct qsound_native_source_bracket {
    const qsound_native_source_frame* lower = nullptr;
    const qsound_native_source_frame* upper = nullptr;
    std::uint32_t fraction_numerator = 0;
    std::uint32_t fraction_denominator = 0;

    bool available() const noexcept {
        return lower != nullptr && upper != nullptr && fraction_denominator != 0;
    }
};

// Bounded cross-block source history for the shared QSound native timeline.
// It answers which exact native frames surround one consumer-time coordinate.
// It performs no interpolation and invents no missing startup/history sample.
class qsound_native_source_window {
public:
    void reset() noexcept {
        history_count_ = 0;
        block_frames_ = nullptr;
        block_count_ = 0;
        block_open_ = false;
        invalid_ = false;
    }

    bool begin_block(
        const qsound_native_source_frame* frames,
        std::size_t count) noexcept
    {
        if (invalid_ || block_open_ || (count != 0 && frames == nullptr)) {
            invalid_ = true;
            return false;
        }

        for (std::size_t i = 1; i < count; ++i) {
            if (frames[i - 1].native_sample == std::numeric_limits<std::uint64_t>::max() ||
                frames[i].native_sample != frames[i - 1].native_sample + 1) {
                invalid_ = true;
                return false;
            }
        }

        if (history_count_ != 0 && count != 0) {
            const std::uint64_t previous = history_[history_count_ - 1].native_sample;
            if (previous == std::numeric_limits<std::uint64_t>::max() ||
                frames[0].native_sample != previous + 1) {
                invalid_ = true;
                return false;
            }
        }

        block_frames_ = frames;
        block_count_ = count;
        block_open_ = true;
        return true;
    }

    bool find_bracket(
        const qsound_native_time_point& point,
        qsound_native_source_bracket& out) const noexcept
    {
        out = {};
        if (invalid_ || !block_open_ || point.fraction_denominator == 0 ||
            point.fraction_numerator >= point.fraction_denominator)
            return false;

        const qsound_native_source_frame* lower = find(point.native_floor);
        if (lower == nullptr)
            return false;

        const qsound_native_source_frame* upper = lower;
        if (point.fraction_numerator != 0) {
            if (point.native_floor == std::numeric_limits<std::uint64_t>::max())
                return false;
            upper = find(point.native_floor + 1);
            if (upper == nullptr)
                return false;
        }

        out.lower = lower;
        out.upper = upper;
        out.fraction_numerator = point.fraction_numerator;
        out.fraction_denominator = point.fraction_denominator;
        return true;
    }

    bool end_block() noexcept {
        if (invalid_ || !block_open_) {
            invalid_ = true;
            return false;
        }

        retain_latest();
        block_frames_ = nullptr;
        block_count_ = 0;
        block_open_ = false;
        return true;
    }

    bool valid() const noexcept { return !invalid_; }
    std::size_t history_count() const noexcept { return history_count_; }

private:
    const qsound_native_source_frame* find(std::uint64_t native_sample) const noexcept {
        for (std::size_t i = 0; i < history_count_; ++i) {
            if (history_[i].native_sample == native_sample)
                return &history_[i];
        }

        if (block_count_ == 0)
            return nullptr;
        const std::uint64_t first = block_frames_[0].native_sample;
        if (native_sample < first)
            return nullptr;
        const std::uint64_t offset = native_sample - first;
        if (offset >= block_count_)
            return nullptr;
        return &block_frames_[static_cast<std::size_t>(offset)];
    }

    void retain_latest() noexcept {
        if (block_count_ >= 2) {
            history_[0] = block_frames_[block_count_ - 2];
            history_[1] = block_frames_[block_count_ - 1];
            history_count_ = 2;
            return;
        }

        if (block_count_ == 1) {
            if (history_count_ == 0) {
                history_[0] = block_frames_[0];
                history_count_ = 1;
            } else if (history_count_ == 1) {
                history_[1] = block_frames_[0];
                history_count_ = 2;
            } else {
                history_[0] = history_[1];
                history_[1] = block_frames_[0];
            }
        }
    }

    std::array<qsound_native_source_frame, 2> history_{};
    std::size_t history_count_ = 0;
    const qsound_native_source_frame* block_frames_ = nullptr;
    std::size_t block_count_ = 0;
    bool block_open_ = false;
    bool invalid_ = false;
};

} // namespace gameaudio::vgm
