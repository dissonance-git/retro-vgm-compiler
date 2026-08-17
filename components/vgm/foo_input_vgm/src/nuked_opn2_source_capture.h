#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// Internal extension point for the pinned libvgm Nuked OPN2 core.
//
// The normal ym3438_update() path clocks the exact same state through
// OPN2_GenerateStream(). The helpers here walk that public/pinned state once,
// preserving the 24-cycle output bus, MD1 low-pass, and native resampling while
// exposing six FM channel contributions plus DAC before the ordinary stereo sum.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_fm_channels = 6;
static constexpr std::size_t opn2_source_count = 7; // FM1..FM6 + DAC

struct nuked_opn2_source_resampler {
    Bit32s sample[2]{};
    Bit32s old_sample[2]{};
    Bit64u samplecnt = 0;
    Bit64u rateratio = 0;

    std::array<std::array<Bit32s, 2>, opn2_source_count> source_sample{};
    std::array<std::array<Bit32s, 2>, opn2_source_count> old_source_sample{};
    std::array<std::array<Bit32s, 2>, opn2_source_count> native_accumulator{};
    std::array<std::array<Bit32s, 2>, opn2_source_count> md1_last{};
    std::array<Bit32s, 2> md1_mix_last{};

    void reset() noexcept { *this = {}; }
};

inline std::size_t opn2_output_cycle_channel(Bit32u cycles) noexcept
{
    // Mirrors OPN2_DoIO's output multiplexing. The output bus emits a channel's
    // slot-group contribution once per 24 internal cycles.
    switch (cycles >> 2)
    {
    case 0: return 1; // Ch 2
    case 1: return 5; // Ch 6 / DAC
    case 2: return 3; // Ch 4
    case 3: return 0; // Ch 1
    case 4: return 4; // Ch 5
    case 5: return 2; // Ch 3
    default: return 0;
    }
}

inline void opn2_clock_sources(
    ym3438_t* chip,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    mix[0] = mix[1] = 0;
    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        lanes[lane][0] = lanes[lane][1] = 0;

    for (Bit32u cycles = 0; cycles < 24; ++cycles)
    {
        Bit32s pins[2]{};
        OPN2_Clock(chip, pins);
        mix[0] += pins[0];
        mix[1] += pins[1];

        const std::size_t channel = opn2_output_cycle_channel(cycles);
        // Nuked internally mutes channel 6 FM while DAC data is active. Its
        // output bus still belongs to one physical channel slot, so attribute
        // that slot to the separate DAC identity whenever DAC is enabled.
        const std::size_t lane = (channel == 5 && chip->dac_en) ? 6 : channel;
        lanes[lane][0] += pins[0];
        lanes[lane][1] += pins[1];
    }
}

inline void opn2_apply_md1_lowpass(
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    for (std::size_t side = 0; side < 2; ++side)
    {
        Bit64s source_sum = 0;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            const Bit32s filtered = (state.md1_last[lane][side] + lanes[lane][side]) >> 1;
            state.md1_last[lane][side] = lanes[lane][side];
            lanes[lane][side] = filtered;
            source_sum += filtered;
        }

        const Bit32s filtered_mix = (state.md1_mix_last[side] + mix[side]) >> 1;
        state.md1_mix_last[side] = mix[side];
        mix[side] = filtered_mix;
        (void)source_sum;
    }
}

inline void opn2_generate_native_sources(
    ym3438_t* chip,
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    opn2_clock_sources(chip, mix, lanes);
    if (chip->chip_type & ym3438_mode_ym2612)
        opn2_apply_md1_lowpass(state, mix, lanes);
}

inline void opn2_update_ratio(ym3438_t* chip, nuked_opn2_source_resampler& state) noexcept
{
    if (state.rateratio == 0)
    {
        const Bit64u chip_rate = static_cast<Bit64u>(chip->clock) / 144u;
        state.rateratio = chip->rate != 0
            ? (chip_rate << RSM_FRAC) / chip->rate
            : 0;
    }
}

inline void generate_nuked_opn2_sources(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    opn2_update_ratio(&chip, state);
    if (state.rateratio == 0)
    {
        mix[0] = mix[1] = 0;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
            lanes[lane][0] = lanes[lane][1] = 0;
        return;
    }

    while (state.samplecnt >= state.rateratio)
    {
        state.old_sample[0] = state.sample[0];
        state.old_sample[1] = state.sample[1];
        state.sample[0] = state.sample[1] = 0;

        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            state.old_source_sample[lane] = state.source_sample[lane];
            state.source_sample[lane] = {0, 0};
        }

        while (state.samplecnt >= state.rateratio)
        {
            Bit32s native_mix[2]{};
            Bit32s native_sources[opn2_source_count][2]{};
            opn2_generate_native_sources(&chip, state, native_mix, native_sources);
            state.sample[0] += native_mix[0];
            state.sample[1] += native_mix[1];
            for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
            {
                state.source_sample[lane][0] += native_sources[lane][0];
                state.source_sample[lane][1] += native_sources[lane][1];
            }
            state.samplecnt -= state.rateratio;
        }
    }

    const Bit64u frac = state.samplecnt;
    for (std::size_t side = 0; side < 2; ++side)
    {
        const Bit64s value = static_cast<Bit64s>(state.old_sample[side])
            * static_cast<Bit64s>(state.rateratio - frac)
            + static_cast<Bit64s>(state.sample[side]) * static_cast<Bit64s>(frac);
        mix[side] = static_cast<Bit32s>(value / static_cast<Bit64s>(state.rateratio));
    }

    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
    {
        for (std::size_t side = 0; side < 2; ++side)
        {
            const Bit64s value = static_cast<Bit64s>(state.old_source_sample[lane][side])
                * static_cast<Bit64s>(state.rateratio - frac)
                + static_cast<Bit64s>(state.source_sample[lane][side]) * static_cast<Bit64s>(frac);
            lanes[lane][side] = static_cast<Bit32s>(
                value / static_cast<Bit64s>(state.rateratio));
        }
    }

    state.samplecnt += 1u << RSM_FRAC;
}

inline Bit32s source_accounting_residual(
    Bit32s mix,
    const Bit32s lanes[opn2_source_count][2],
    std::size_t side) noexcept
{
    Bit64s sum = 0;
    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        sum += lanes[lane][side];
    return static_cast<Bit32s>(static_cast<Bit64s>(mix) - sum);
}

} // namespace foobar_vgm::genesis
