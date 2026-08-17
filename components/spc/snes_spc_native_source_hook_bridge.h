#pragma once

#include "spc_native_source_capture.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

enum class snes_spc_native_source_hook_error : std::uint8_t {
    none = 0,
    inactive,
    capture_rejected,
};

// Small stateful seam for an instrumented snes_spc/libgme DSP loop.
//
// The upstream accurate DSP has one causal mono `output` value per physical
// voice after interpolation/noise selection, envelope application, and pitch
// modulation, but before signed VOLL/VOLR routing and before the shared echo
// accumulator. A patched DSP collects those eight values for one native 32 kHz
// synthesis instant and calls observe_frame() exactly once.
//
// This bridge owns only the monotonically increasing native-sample ordinal.
// Source amplitudes and validity remain owned by spc_native_source_capture.
// begin_block() clears bounded storage without restarting the ordinal; reset()
// is the explicit seek/track/reset boundary that may establish a new ordinal.
class snes_spc_native_source_hook_bridge {
public:
    void reset(
        spc_native_source_capture* capture,
        std::uint64_t next_native_sample = 0) noexcept
    {
        capture_ = capture;
        next_native_sample_ = next_native_sample;
        active_ = capture_ != nullptr;
        last_error_ = snes_spc_native_source_hook_error::none;
        if (capture_ != nullptr)
            capture_->reset_trace();
    }

    void deactivate() noexcept {
        capture_ = nullptr;
        active_ = false;
        last_error_ = snes_spc_native_source_hook_error::none;
    }

    bool begin_block() noexcept {
        last_error_ = snes_spc_native_source_hook_error::none;
        if (!active_ || capture_ == nullptr)
            return fail(snes_spc_native_source_hook_error::inactive);
        capture_->begin_block();
        return true;
    }

    bool observe_frame(
        const std::int16_t* source,
        std::size_t voice_count = spc_native_voice_count) noexcept
    {
        last_error_ = snes_spc_native_source_hook_error::none;
        if (!active_ || capture_ == nullptr)
            return fail(snes_spc_native_source_hook_error::inactive);

        // Let the capture contract validate the exact frame shape and ordinal,
        // including the terminal uint64 boundary. If it rejects the frame, stop
        // the hook immediately so a later call cannot hide a missing native
        // synthesis instant by reusing the same ordinal.
        capture_->observe(
            spc_native_sample_rate,
            next_native_sample_,
            source,
            voice_count);
        if (!capture_->valid()) {
            active_ = false;
            return fail(snes_spc_native_source_hook_error::capture_rejected);
        }

        ++next_native_sample_;
        return true;
    }

    bool active() const noexcept { return active_; }
    std::uint64_t next_native_sample() const noexcept { return next_native_sample_; }
    snes_spc_native_source_hook_error last_error() const noexcept { return last_error_; }

private:
    bool fail(snes_spc_native_source_hook_error error) noexcept {
        last_error_ = error;
        return false;
    }

    spc_native_source_capture* capture_ = nullptr;
    std::uint64_t next_native_sample_ = 0;
    bool active_ = false;
    snes_spc_native_source_hook_error last_error_ =
        snes_spc_native_source_hook_error::none;
};

} // namespace gameaudio::spc
