#pragma once

#include "spc_upstream_playback_reconstruction.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

constexpr std::size_t snesapu_studio_voice_count = 8u;
constexpr std::uint32_t snesapu_brr_samples_per_block = 16u;
constexpr std::uint32_t snesapu_brr_bytes_per_block = 9u;

inline bool snesapu_studio_brr_topology_valid(
    std::uint16_t first_brr_block_address,
    std::uint16_t loop_brr_block_address,
    const spc_game_sample_playback_span& playback) noexcept
{
    std::int64_t end_sample = 0;
    if (!playback.valid()
        || !detail::spc_near_integer(playback.end_sample, end_sample)
        || end_sample <= 0
        || (static_cast<std::uint64_t>(end_sample) % snesapu_brr_samples_per_block) != 0u)
        return false;

    if (!playback.loop.present)
        return true;

    std::int64_t loop_sample = 0;
    if (!detail::spc_near_integer(playback.loop.start_sample, loop_sample)
        || loop_sample < 0
        || loop_sample >= end_sample
        || (static_cast<std::uint64_t>(loop_sample) % snesapu_brr_samples_per_block) != 0u)
        return false;

    const std::uint64_t loop_block = static_cast<std::uint64_t>(loop_sample)
        / snesapu_brr_samples_per_block;
    const std::uint16_t expected_loop = static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(first_brr_block_address)
        + static_cast<std::uint32_t>(loop_block * snesapu_brr_bytes_per_block));
    return expected_loop == loop_brr_block_address;
}

// Setup-time binding between one concrete game sample object and an already
// admitted upstream restoration. The first BRR address is part of the key, and
// the loop BRR address is separately proven against the authored block topology.
// This prevents a runtime DIR/SRCN remap from silently reusing a candidate for
// different bytes or for the same bytes with a different loop target.
struct snesapu_studio_source_binding {
    std::uint8_t source_number = 0;
    std::uint16_t first_brr_block_address = 0;
    std::uint16_t loop_brr_block_address = 0;
    const spc_sample_restoration_candidate* restoration = nullptr;
    spc_game_sample_playback_span playback{};
};

// Child-process realtime owner for the highest-confidence sampled-source path.
// Discovery, file I/O, hashing, provenance work and allocations happen before
// this object is populated. The hot callback is fixed-capacity lookup performed
// once at key-on, followed by one trajectory projection + FIR evaluation per
// mixed voice sample.
template <std::size_t MaxSources = 256>
class snesapu_studio_source_provider {
    static_assert(MaxSources > 0, "MaxSources must be non-zero");

public:
    bool add(const snesapu_studio_source_binding& binding) noexcept {
        if (count_ >= sources_.size() || binding.restoration == nullptr
            || !may_use_spc_sample_restoration_automatically(*binding.restoration)
            || !binding.playback.valid()
            || !snesapu_studio_brr_topology_valid(
                    binding.first_brr_block_address,
                    binding.loop_brr_block_address,
                    binding.playback)
            || !detail::resolve_spc_upstream_playback_boundaries(
                    *binding.restoration, binding.playback).valid)
            return false;

        // A concrete runtime source identity must resolve exactly once. If the
        // setup layer has competing hypotheses, adjudicate them before audio.
        for (std::size_t index = 0; index < count_; ++index) {
            if (sources_[index].source_number == binding.source_number
                && sources_[index].first_brr_block_address
                    == binding.first_brr_block_address)
                return false;
        }

        sources_[count_++] = binding;
        return true;
    }

    void clear() noexcept {
        sources_ = {};
        voices_ = {};
        count_ = 0;
    }

    std::size_t source_count() const noexcept { return count_; }

    bool begin_voice(
        std::uint32_t voice,
        std::uint32_t source_number,
        std::uint32_t first_brr_block_address,
        std::uint32_t loop_brr_block_address,
        std::uint32_t directory_page,
        std::uint32_t interpolation_raw) noexcept
    {
        if (voice >= voices_.size())
            return false;
        stop_voice(voice);
        if (source_number > 0xffu || first_brr_block_address > 0xffffu
            || loop_brr_block_address > 0xffffu || directory_page > 0xffu)
            return false;

        snesapu_source_interpolation interpolation{};
        if (!snesapu_source_interpolation_from_raw(interpolation_raw, interpolation))
            return false;

        const snesapu_studio_source_binding* selected = nullptr;
        for (std::size_t index = 0; index < count_; ++index) {
            const auto& candidate = sources_[index];
            if (candidate.source_number == static_cast<std::uint8_t>(source_number)
                && candidate.first_brr_block_address
                    == static_cast<std::uint16_t>(first_brr_block_address)) {
                selected = &candidate;
                break;
            }
        }
        if (selected == nullptr
            || (selected->playback.loop.present
                && selected->loop_brr_block_address
                    != static_cast<std::uint16_t>(loop_brr_block_address)))
            return false;

        auto& state = voices_[voice];
        state.source = selected;
        state.directory_page = static_cast<std::uint8_t>(directory_page);
        state.trajectory.key_on(interpolation);
        state.active = true;
        return true;
    }

    bool render_voice(
        std::uint32_t voice,
        std::uint32_t m_rate_q16_16,
        std::uint32_t effective_source_number,
        std::uint32_t live_loop_brr_block_address,
        std::uint32_t directory_page,
        std::uint32_t interpolation_raw,
        float* output_sample) noexcept
    {
        if (voice >= voices_.size() || output_sample == nullptr)
            return false;
        auto& state = voices_[voice];
        if (!state.active || state.source == nullptr)
            return false;

        snesapu_source_interpolation interpolation{};
        if (effective_source_number > 0xffu
            || live_loop_brr_block_address > 0xffffu
            || directory_page > 0xffu
            || static_cast<std::uint8_t>(effective_source_number)
                != state.source->source_number
            || (state.source->playback.loop.present
                && static_cast<std::uint16_t>(live_loop_brr_block_address)
                    != state.source->loop_brr_block_address)
            || static_cast<std::uint8_t>(directory_page) != state.directory_page
            || !snesapu_source_interpolation_from_raw(interpolation_raw, interpolation)
            || !snesapu_source_rate_representable(m_rate_q16_16))
            return stop_and_fail(voice);

        // The pinned SNESAPU loop-restart path may refresh mSrc from the live
        // DSP SRCN, apply Script700 NoteChange again, and re-read that source's
        // loop pointer from current DIR RAM. The assembly callback therefore
        // supplies the live effective SRCN and loop pointer on every restored
        // sample. A dynamic remap is rejected before the first post-remap studio
        // sample can use the old authored trajectory.
        //
        // SetDSPOpt may also change the historical interpolation routine while a
        // voice is alive. That changes only the pInter timing center, so keep the
        // accumulated source phase and update the center without resetting it.
        state.trajectory.set_interpolation(interpolation);
        const auto projection = state.trajectory.project(state.source->playback.loop);
        if (!projection.valid)
            return stop_and_fail(voice);

        double sample = 0.0;
        if (!projection.before_key_on) {
            const auto reconstructed = reconstruct_spc_upstream_playback_sample(
                *state.source->restoration,
                state.source->playback,
                projection);
            if (!reconstructed.valid || !std::isfinite(reconstructed.sample))
                return stop_and_fail(voice);
            sample = reconstructed.sample;
        }

        const float narrowed = static_cast<float>(sample);
        if (!std::isfinite(narrowed))
            return stop_and_fail(voice);

        // MixSample runs before UpdateSrc. Advancing here after evaluating the
        // current phase makes the provider's next call observe exactly the same
        // recurrence as SNESAPU's following mDec/sIdx update, including PMON.
        if (!state.trajectory.advance(m_rate_q16_16))
            return stop_and_fail(voice);

        *output_sample = narrowed;
        return true;
    }

    void stop_voice(std::uint32_t voice) noexcept {
        if (voice >= voices_.size())
            return;
        voices_[voice] = {};
    }

    bool voice_active(std::uint32_t voice) const noexcept {
        return voice < voices_.size() && voices_[voice].active;
    }

    static std::uint32_t begin_callback(
        void* user,
        std::uint32_t voice,
        std::uint32_t source_number,
        std::uint32_t first_brr_block_address,
        std::uint32_t loop_brr_block_address,
        std::uint32_t directory_page,
        std::uint32_t interpolation) noexcept
    {
        if (user == nullptr)
            return 0u;
        auto* self = static_cast<snesapu_studio_source_provider*>(user);
        return self->begin_voice(
            voice,
            source_number,
            first_brr_block_address,
            loop_brr_block_address,
            directory_page,
            interpolation) ? 1u : 0u;
    }

    static std::uint32_t sample_callback(
        void* user,
        std::uint32_t voice,
        std::uint32_t m_rate_q16_16,
        std::uint32_t effective_source_number,
        std::uint32_t live_loop_brr_block_address,
        std::uint32_t directory_page,
        std::uint32_t interpolation,
        float* output_sample) noexcept
    {
        if (user == nullptr)
            return 0u;
        auto* self = static_cast<snesapu_studio_source_provider*>(user);
        return self->render_voice(
            voice,
            m_rate_q16_16,
            effective_source_number,
            live_loop_brr_block_address,
            directory_page,
            interpolation,
            output_sample) ? 1u : 0u;
    }

private:
    struct voice_state {
        const snesapu_studio_source_binding* source = nullptr;
        snesapu_source_trajectory_tracker trajectory{};
        std::uint8_t directory_page = 0;
        bool active = false;
    };

    bool stop_and_fail(std::uint32_t voice) noexcept {
        stop_voice(voice);
        return false;
    }

    std::array<snesapu_studio_source_binding, MaxSources> sources_{};
    std::array<voice_state, snesapu_studio_voice_count> voices_{};
    std::size_t count_ = 0;
};

} // namespace gameaudio::spc
