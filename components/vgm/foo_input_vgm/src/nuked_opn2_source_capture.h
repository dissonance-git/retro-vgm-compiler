#pragma once

#include "nuked_opn2_hq_lift.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// Decomposition of the pinned NOPN2_GenerateResampled path. The authoritative
// ym3438_t advances exactly once. Alongside exact six-FM-plus-DAC reference
// lanes, the same cycles can produce six identity-conservative HQ FM lanes from
// exact live OPN phase/envelope/modulation state.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_fm_channels = 6;
static constexpr std::size_t opn2_source_count = 7; // FM1..FM6 + DAC
static constexpr double opn2_md1_filter_cutoff = 0.512331301282628;
static constexpr double opn2_md1_filter_remainder = 1.0 - opn2_md1_filter_cutoff;

struct nuked_opn2_source_resampler {
    std::array<std::array<Bit32s, 2>, opn2_source_count> source_sample{};
    std::array<std::array<Bit32s, 2>, opn2_source_count> old_source_sample{};

    // HQ FM deliberately bypasses the historical model-1 output low-pass and
    // DAC-ladder/sign-leak ceiling, so its native history is separate from the
    // exact reference lane history.
    nuked_opn2_hq_lift_state hq_lift{};
    std::array<std::array<double, 2>, opn2_fm_channels> hq_source_sample{};
    std::array<std::array<double, 2>, opn2_fm_channels> old_hq_source_sample{};

    void reset() noexcept { *this = {}; }
};

inline std::size_t opn2_output_cycle_channel(Bit32u cycles) noexcept
{
    switch (cycles >> 2)
    {
    case 0: return 1;
    case 1: return 5;
    case 2: return 3;
    case 3: return 0;
    case 4: return 4;
    case 5: return 2;
    default: return 0;
    }
}

inline Bit32u opn2_output_cycle_mute(const ym3438_t& chip, Bit32u cycles) noexcept
{
    switch (cycles >> 2)
    {
    case 0: return chip.mute[1];
    case 1: return chip.mute[5 + (chip.dacen ? 1u : 0u)];
    case 2: return chip.mute[3];
    case 3: return chip.mute[0];
    case 4: return chip.mute[4];
    case 5: return chip.mute[2];
    default: return 0;
    }
}

inline void opn2_apply_due_buffered_writes(ym3438_t& chip) noexcept
{
    while (chip.writebuf[chip.writebuf_cur].time <= chip.writebuf_samplecnt)
    {
        if (!(chip.writebuf[chip.writebuf_cur].port & 0x04))
            break;
        chip.writebuf[chip.writebuf_cur].port &= 0x03;
        NOPN2_Write(
            &chip,
            chip.writebuf[chip.writebuf_cur].port,
            chip.writebuf[chip.writebuf_cur].data);
        chip.writebuf_cur = (chip.writebuf_cur + 1) % NOPN_WRITEBUF_SIZE;
    }
    ++chip.writebuf_samplecnt;
}

inline void opn2_clock_native_source_frame(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s raw_mix[2],
    Bit32s raw_lanes[opn2_source_count][2],
    double raw_hq_lanes[opn2_fm_channels][2]) noexcept
{
    raw_mix[0] = raw_mix[1] = 0;
    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        raw_lanes[lane][0] = raw_lanes[lane][1] = 0;
    for (std::size_t lane = 0; lane < opn2_fm_channels; ++lane)
        raw_hq_lanes[lane][0] = raw_hq_lanes[lane][1] = 0.0;

    for (Bit32u step = 0; step < 24; ++step)
    {
        (void)step;
        const Bit32u cycle = chip.cycles;
        const std::size_t channel = opn2_output_cycle_channel(cycle);
        const bool dac_lane = channel == 5 && chip.dacen != 0;
        const std::size_t lane = dac_lane ? 6u : channel;
        const Bit32u mute = opn2_output_cycle_mute(chip, cycle);

        const auto pending_hq = opn2_hq_prepare_cycle(
            chip, state.hq_lift, raw_hq_lanes);

        Bit32s pins[2]{};
        NOPN2_Clock(&chip, pins);
        opn2_hq_finish_cycle(pending_hq, state.hq_lift);

        if (!mute)
        {
            raw_mix[0] += pins[0];
            raw_mix[1] += pins[1];
            raw_lanes[lane][0] += pins[0];
            raw_lanes[lane][1] += pins[1];
        }

        opn2_apply_due_buffered_writes(chip);
    }
}

inline void opn2_finish_native_source_frame(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s raw_lanes[opn2_source_count][2],
    double raw_hq_lanes[opn2_fm_channels][2]) noexcept
{
    // HQ FM uses the same broad output scale as Nuked's no-filter path while
    // intentionally skipping the historical MD1 analog low-pass.
    for (std::size_t lane = 0; lane < opn2_fm_channels; ++lane)
    {
        state.hq_source_sample[lane][0] = raw_hq_lanes[lane][0] * 11.0;
        state.hq_source_sample[lane][1] = raw_hq_lanes[lane][1] * 11.0;
    }

    if (!chip.use_filter)
    {
        chip.samples[0] *= 11;
        chip.samples[1] *= 11;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            state.source_sample[lane][0] = raw_lanes[lane][0] * 11;
            state.source_sample[lane][1] = raw_lanes[lane][1] * 11;
        }
        return;
    }

    for (std::size_t side = 0; side < 2; ++side)
    {
        chip.samples[side] = chip.oldsamples[side] + static_cast<Bit32s>(
            opn2_md1_filter_remainder
            * (static_cast<double>(chip.samples[side]) * 12.0
               - static_cast<double>(chip.oldsamples[side])));

        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            const Bit32s previous = state.old_source_sample[lane][side];
            state.source_sample[lane][side] = previous + static_cast<Bit32s>(
                opn2_md1_filter_remainder
                * (static_cast<double>(raw_lanes[lane][side]) * 12.0
                   - static_cast<double>(previous)));
        }
    }
}

inline Bit32s opn2_round_hq_to_source_unit(double value) noexcept
{
    if (!std::isfinite(value))
        return 0;
    const double lo = static_cast<double>(std::numeric_limits<Bit32s>::min());
    const double hi = static_cast<double>(std::numeric_limits<Bit32s>::max());
    if (value <= lo) return std::numeric_limits<Bit32s>::min();
    if (value >= hi) return std::numeric_limits<Bit32s>::max();
    return static_cast<Bit32s>(std::llround(value));
}

inline void generate_nuked_opn2_sources(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2],
    Bit32s hq_fm_lanes[opn2_fm_channels][2]) noexcept
{
    if (chip.rateratio <= 0)
    {
        mix[0] = mix[1] = 0;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
            lanes[lane][0] = lanes[lane][1] = 0;
        for (std::size_t lane = 0; lane < opn2_fm_channels; ++lane)
            hq_fm_lanes[lane][0] = hq_fm_lanes[lane][1] = 0;
        return;
    }

    while (chip.samplecnt >= chip.rateratio)
    {
        chip.oldsamples[0] = chip.samples[0];
        chip.oldsamples[1] = chip.samples[1];
        chip.samples[0] = chip.samples[1] = 0;

        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            state.old_source_sample[lane] = state.source_sample[lane];
            state.source_sample[lane] = {0, 0};
        }
        for (std::size_t lane = 0; lane < opn2_fm_channels; ++lane)
        {
            state.old_hq_source_sample[lane] = state.hq_source_sample[lane];
            state.hq_source_sample[lane] = {0.0, 0.0};
        }

        Bit32s raw_mix[2]{};
        Bit32s raw_lanes[opn2_source_count][2]{};
        double raw_hq_lanes[opn2_fm_channels][2]{};
        opn2_clock_native_source_frame(
            chip, state, raw_mix, raw_lanes, raw_hq_lanes);
        chip.samples[0] = raw_mix[0];
        chip.samples[1] = raw_mix[1];
        opn2_finish_native_source_frame(
            chip, state, raw_lanes, raw_hq_lanes);
        chip.samplecnt -= chip.rateratio;
    }

    const Bit32s frac = chip.samplecnt;
    for (std::size_t side = 0; side < 2; ++side)
    {
        const Bit64s value = static_cast<Bit64s>(chip.oldsamples[side])
            * static_cast<Bit64s>(chip.rateratio - frac)
            + static_cast<Bit64s>(chip.samples[side]) * static_cast<Bit64s>(frac);
        mix[side] = static_cast<Bit32s>(value / static_cast<Bit64s>(chip.rateratio));
    }

    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
    {
        for (std::size_t side = 0; side < 2; ++side)
        {
            const Bit64s value = static_cast<Bit64s>(state.old_source_sample[lane][side])
                * static_cast<Bit64s>(chip.rateratio - frac)
                + static_cast<Bit64s>(state.source_sample[lane][side])
                    * static_cast<Bit64s>(frac);
            lanes[lane][side] = static_cast<Bit32s>(
                value / static_cast<Bit64s>(chip.rateratio));
        }
    }

    for (std::size_t lane = 0; lane < opn2_fm_channels; ++lane)
    {
        for (std::size_t side = 0; side < 2; ++side)
        {
            const double value = (
                state.old_hq_source_sample[lane][side]
                    * static_cast<double>(chip.rateratio - frac)
                + state.hq_source_sample[lane][side]
                    * static_cast<double>(frac))
                / static_cast<double>(chip.rateratio);
            hq_fm_lanes[lane][side] = opn2_round_hq_to_source_unit(value);
        }
    }

    chip.samplecnt += 1 << RSM_FRAC;
}

// Compatibility overload for consumers that only need exact decomposition.
inline void generate_nuked_opn2_sources(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    Bit32s ignored_hq[opn2_fm_channels][2]{};
    generate_nuked_opn2_sources(chip, state, mix, lanes, ignored_hq);
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
