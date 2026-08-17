#pragma once

#include "snes_spc_native_source_hook_bridge.h"
#include "spc_enhanced_reconstruction.h"
#include "spc_upstream_sample_reconstruction.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Exact source-domain facts required to replace only sampled-voice
// reconstruction. Noise bypasses the BRR reconstruction filter in the S-DSP,
// so a noise voice carries its exact pre-envelope noise sample instead.
struct spc_enhanced_voice_input {
    spc_decoded_source_window decoded{};
    std::uint16_t envelope = 0;
    bool noise_enabled = false;
    std::int16_t noise_sample = 0;

    // Optional evidence-approved upstream source replacement. The producer owns
    // exact game-sample traversal and supplies its current decoded-sample
    // coordinate after the game's pitch/loop logic. If a candidate is supplied,
    // failure to reconstruct it is a hard Enhanced-source failure rather than a
    // silent fallback to BRR for that frame.
    const spc_sample_restoration_candidate* restoration = nullptr;
    double game_sample_position = 0.0;
};

enum class snes_spc_enhanced_source_hook_error : std::uint8_t {
    none = 0,
    inactive,
    invalid_voice_count,
    reconstruction_rejected,
    restoration_rejected,
    downstream_rejected,
};

// Adapter for an instrumented libgme/snes_spc DSP loop. The dependency exposes
// decoded BRR neighborhoods and q12 source phase before its native Gaussian
// interpolation. This bridge reconstructs sampled voices with the bounded
// enhanced filter or an independently proven upstream source, deliberately
// retains native envelope quantization for the first experiment, and feeds the
// resulting pre-pan source frame into the same exact source transport already
// used by the reference hook.
//
// This keeps the rest of the realtime pipeline agnostic to whether the source
// sample came from protected BRR evidence or the independently enabled Enhanced
// path. Spatial presentation therefore remains orthogonal.
class snes_spc_enhanced_source_hook_bridge {
public:
    void reset(
        spc_native_source_capture* capture,
        std::uint64_t next_native_sample = 0) noexcept
    {
        output_bridge_.reset(capture, next_native_sample);
        active_ = capture != nullptr;
        last_error_ = snes_spc_enhanced_source_hook_error::none;
    }

    void deactivate() noexcept {
        output_bridge_.deactivate();
        active_ = false;
        last_error_ = snes_spc_enhanced_source_hook_error::none;
    }

    bool begin_block() noexcept {
        last_error_ = snes_spc_enhanced_source_hook_error::none;
        if (!active_)
            return fail(snes_spc_enhanced_source_hook_error::inactive);
        if (!output_bridge_.begin_block()) {
            active_ = false;
            return fail(snes_spc_enhanced_source_hook_error::downstream_rejected);
        }
        return true;
    }

    bool observe_frame(
        const spc_enhanced_voice_input* voices,
        std::size_t voice_count = spc_native_voice_count) noexcept
    {
        last_error_ = snes_spc_enhanced_source_hook_error::none;
        if (!active_ || voices == nullptr)
            return fail(snes_spc_enhanced_source_hook_error::inactive);
        if (voice_count != spc_native_voice_count)
            return fail(snes_spc_enhanced_source_hook_error::invalid_voice_count);

        std::array<std::int16_t, spc_native_voice_count> source{};
        for (std::size_t voice = 0; voice < spc_native_voice_count; ++voice) {
            double pre_envelope = static_cast<double>(voices[voice].noise_sample);
            if (!voices[voice].noise_enabled) {
                if (voices[voice].restoration != nullptr) {
                    const auto restored = reconstruct_spc_upstream_sample(
                        *voices[voice].restoration,
                        voices[voice].game_sample_position);
                    if (!restored.valid)
                        return stop(snes_spc_enhanced_source_hook_error::restoration_rejected);
                    pre_envelope = restored.sample;
                } else {
                    const auto reconstructed = reconstruct_spc_lanczos4(voices[voice].decoded);
                    if (!reconstructed.valid)
                        return stop(snes_spc_enhanced_source_hook_error::reconstruction_rejected);
                    pre_envelope = reconstructed.sample;
                }
            }

            source[voice] = apply_spc_reference_envelope_quantization(
                pre_envelope,
                voices[voice].envelope);
        }

        if (!output_bridge_.observe_frame(source.data(), source.size())) {
            active_ = false;
            return fail(snes_spc_enhanced_source_hook_error::downstream_rejected);
        }
        return true;
    }

    bool active() const noexcept { return active_ && output_bridge_.active(); }
    std::uint64_t next_native_sample() const noexcept {
        return output_bridge_.next_native_sample();
    }
    snes_spc_enhanced_source_hook_error last_error() const noexcept {
        return last_error_;
    }

private:
    bool stop(snes_spc_enhanced_source_hook_error error) noexcept {
        active_ = false;
        output_bridge_.deactivate();
        return fail(error);
    }

    bool fail(snes_spc_enhanced_source_hook_error error) noexcept {
        last_error_ = error;
        return false;
    }

    snes_spc_native_source_hook_bridge output_bridge_{};
    bool active_ = false;
    snes_spc_enhanced_source_hook_error last_error_ =
        snes_spc_enhanced_source_hook_error::none;
};

} // namespace gameaudio::spc
