#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// Internal extension point for the pinned libvgm Nuked OPN2 core.
//
// This is a decomposition of the pinned NOPN2_GenerateResampled path, not a
// second emulator. It advances the one authoritative ym3438_t exactly once,
// preserves the write-buffer scheduler, mute behavior, 24-cycle output bus,
// optional MD1 filter and native linear resampler, while accounting the same
// output into six FM lanes plus DAC.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_fm_channels = 6;
static constexpr std::size_t opn2_source_count = 7; // FM1..FM6 + DAC

// Pinned Nuked OPN2 MD1 model-1 filter constants from ym3438.c.
static constexpr double opn2_md1_filter_cutoff = 0.512331301282628;
static constexpr double opn2_md1_filter_remainder = 1.0 - opn2_md1_filter_cutoff;

struct nuked_opn2_source_resampler {
    // These sidecars mirror only source decomposition state. The authoritative
    // mix resampler state remains chip->samplecnt/rateratio/oldsamples/samples.
    std::array<std::array<Bit32s, 2>, opn2_source_count> source_sample{};
    std::array<std::array<Bit32s, 2>, opn2_source_count> old_source_sample{};

    void reset() noexcept { *this = {}; }
};

inline std::size_t opn2_output_cycle_channel(Bit32u cycles) noexcept
{
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
    // Mirrors the exact scheduler in pinned NOPN2_GenerateResampled(). A custom
    // StreamUpdate that omitted this step would silently stop register writes
    // from reaching the chip, so it is part of audio correctness, not metadata.
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
    Bit32s raw_mix[2],
    Bit32s raw_lanes[opn2_source_count][2]) noexcept
{
    raw_mix[0] = raw_mix[1] = 0;
    for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        raw_lanes[lane][0] = raw_lanes[lane][1] = 0;

    for (Bit32u step = 0; step < 24; ++step)
    {
        (void)step;
        const Bit32u cycle = chip.cycles;
        const std::size_t channel = opn2_output_cycle_channel(cycle);
        const bool dac_lane = channel == 5 && chip.dacen != 0;
        const std::size_t lane = dac_lane ? 6u : channel;
        const Bit32u mute = opn2_output_cycle_mute(chip, cycle);

        Bit32s pins[2]{};
        NOPN2_Clock(&chip, pins);
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
    Bit32s raw_lanes[opn2_source_count][2]) noexcept
{
    if (!chip.use_filter)
    {
        // Exact pinned Nuked source scales its 24-cycle accumulated bus by 11
        // when the MD1 low-pass option is disabled.
        chip.samples[0] *= 11;
        chip.samples[1] *= 11;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
        {
            state.source_sample[lane][0] = raw_lanes[lane][0] * 11;
            state.source_sample[lane][1] = raw_lanes[lane][1] * 11;
        }
        return;
    }

    // The historical optional MD1 filter is linear apart from integer rounding.
    // Filter each source with the same recurrence. A tiny accounting residual is
    // possible because per-source rounding is not algebraically identical to
    // one rounded sum; the host validates that residual rather than hiding it.
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

inline void generate_nuked_opn2_sources(
    ym3438_t& chip,
    nuked_opn2_source_resampler& state,
    Bit32s mix[2],
    Bit32s lanes[opn2_source_count][2]) noexcept
{
    if (chip.rateratio <= 0)
    {
        mix[0] = mix[1] = 0;
        for (std::size_t lane = 0; lane < opn2_source_count; ++lane)
            lanes[lane][0] = lanes[lane][1] = 0;
        return;
    }

    // This loop is intentionally the same orientation as pinned Nuked:
    // rateratio = output_rate / native_rate in RSM_FRAC fixed point, while
    // samplecnt advances by one output sample (1 << RSM_FRAC).
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

        Bit32s raw_mix[2]{};
        Bit32s raw_lanes[opn2_source_count][2]{};
        opn2_clock_native_source_frame(chip, raw_mix, raw_lanes);
        chip.samples[0] = raw_mix[0];
        chip.samples[1] = raw_mix[1];
        opn2_finish_native_source_frame(chip, state, raw_lanes);
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

    chip.samplecnt += 1 << RSM_FRAC;
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
