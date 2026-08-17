#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

// The S-DSP synthesizes one stereo hardware frame at 32 kHz. libgme preserves
// that native cadence and resamples only afterwards when a consumer requests a
// different output rate.
constexpr std::uint32_t spc_native_sample_rate = 32000;
constexpr std::size_t spc_native_voice_count = 8;

// One exact S-DSP synthesis instant before per-voice signed VOLL/VOLR routing
// and before contributions enter the shared echo accumulator.
//
// `source[voice]` is the causal mono voice value after BRR/noise selection,
// interpolation, envelope application, and pitch-modulation consequences. It
// is deliberately device-native amplitude, not a normalized final-mix stem.
struct spc_native_source_frame {
    std::uint64_t native_sample = 0;
    std::array<std::int16_t, spc_native_voice_count> source{};
};

// Fixed-capacity realtime capture intended to be called directly from the DSP
// synthesis loop. The callback path performs no allocation, interpolation,
// source-role inference, pan reconstruction, or effect attribution.
//
// A missing/out-of-order frame invalidates the capture window. Downstream code
// must not silently sort, interpolate, or synthesize the missing native sample.
class spc_native_source_capture {
public:
    static constexpr std::size_t voice_count = spc_native_voice_count;
    static constexpr std::size_t capacity = 4096;

    void reset_trace() noexcept {
        begin_block();
        expected_native_sample_valid_ = false;
        expected_native_sample_ = 0;
    }

    void begin_block() noexcept {
        count_ = 0;
        overflowed_ = false;
        invalid_ = false;
    }

    void observe(
        std::uint32_t sample_rate,
        std::uint64_t native_sample,
        const std::int16_t* source,
        std::size_t count) noexcept
    {
        if (invalid_ || overflowed_)
            return;

        if (sample_rate != spc_native_sample_rate || source == nullptr ||
            count != voice_count) {
            invalid_ = true;
            return;
        }

        if (expected_native_sample_valid_ && native_sample != expected_native_sample_) {
            invalid_ = true;
            return;
        }

        if (native_sample == std::numeric_limits<std::uint64_t>::max()) {
            // The frame itself is representable but no coherent following frame
            // exists. Treat this as a terminal invalid boundary rather than
            // permitting the continuity counter to wrap to zero.
            invalid_ = true;
            return;
        }

        if (count_ >= capacity) {
            overflowed_ = true;
            invalid_ = true;
            return;
        }

        spc_native_source_frame& frame = frames_[count_++];
        frame.native_sample = native_sample;
        for (std::size_t voice = 0; voice < voice_count; ++voice)
            frame.source[voice] = source[voice];

        expected_native_sample_ = native_sample + 1u;
        expected_native_sample_valid_ = true;
    }

    bool valid() const noexcept { return !invalid_ && !overflowed_; }
    bool overflowed() const noexcept { return overflowed_; }
    std::size_t count() const noexcept { return count_; }
    std::uint32_t native_sample_rate() const noexcept { return spc_native_sample_rate; }

    bool expected_native_sample_valid() const noexcept {
        return expected_native_sample_valid_;
    }
    std::uint64_t expected_native_sample() const noexcept {
        return expected_native_sample_;
    }
    std::uint64_t first_native_sample() const noexcept {
        return count_ != 0 ? frames_[0].native_sample : 0;
    }
    const spc_native_source_frame* frames() const noexcept {
        return frames_.data();
    }

private:
    std::array<spc_native_source_frame, capacity> frames_{};
    std::size_t count_ = 0;
    bool expected_native_sample_valid_ = false;
    std::uint64_t expected_native_sample_ = 0;
    bool overflowed_ = false;
    bool invalid_ = false;
};

// Minimal observer ABI for an instrumented S-DSP/libgme boundary. Keeping this
// independent of Spc_Dsp headers lets a frontend or patched dependency publish
// native source frames without coupling the semantic model to one emulator
// implementation.
using spc_native_source_observer = void (*)(
    void* user,
    std::uint32_t sample_rate,
    std::uint64_t native_sample,
    const std::int16_t* source,
    std::size_t voice_count);

} // namespace gameaudio::spc
