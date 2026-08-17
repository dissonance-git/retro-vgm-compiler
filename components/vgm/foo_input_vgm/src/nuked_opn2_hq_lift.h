#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// Higher-fidelity carrier lift from the one authoritative pinned Nuked OPN2
// state. Nuked still owns register latency, pitch, LFO/PM/AM, CH3 special mode,
// CSM, SSG-EG, keying, envelope timing and modulation history. This layer only
// replaces the final logsin/exp carrier reconstruction and clamped channel/DAC
// output with continuous sine + floating carrier accumulation.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_hq_channel_count = 6;
static constexpr std::size_t opn2_hq_operator_count = 24;
static constexpr double opn2_hq_pi = 3.141592653589793238462643383279502884;

// Exact carrier-connect column from pinned Nuked OPN2 fm_algorithm[][5][].
static constexpr std::uint8_t opn2_carrier_table[4][8] = {
    {0,0,0,0,0,0,0,1},
    {0,0,0,0,0,1,1,1},
    {0,0,0,0,1,1,1,1},
    {1,1,1,1,1,1,1,1},
};

struct nuked_opn2_hq_lift_state {
    std::array<double, opn2_hq_operator_count> operator_output{};
    std::array<double, opn2_hq_channel_count> channel_accumulator{};
    std::array<double, opn2_hq_channel_count> channel_output{};
    double locked_output = 0.0;
    bool locked_left = false;
    bool locked_right = false;

    void reset() noexcept { *this = {}; }
};

struct nuked_opn2_hq_pending_operator {
    std::size_t slot = 0;
    double output = 0.0;
};

inline std::size_t opn2_hq_bus_channel(Bit32u cycles) noexcept {
    switch (cycles >> 2) {
    case 0: return 1;
    case 1: return 5;
    case 2: return 3;
    case 3: return 0;
    case 4: return 4;
    case 5: return 2;
    default: return 0;
    }
}

inline double opn2_hq_operator_from_exact_state(
    const ym3438_t& chip,
    std::size_t slot) noexcept {
    if (slot >= opn2_hq_operator_count)
        return 0.0;

    const Bit32u phase = static_cast<Bit32u>(
        chip.fm_mod[slot] + (chip.pg_phase[slot] >> 10)) & 0x03ffu;

    // eg_out contributes eg_out<<2 to Nuked's logarithmic amplitude. +256 log
    // units halves amplitude, therefore +64 eg_out units halves amplitude.
    const double attenuation = std::exp2(-static_cast<double>(chip.eg_out[slot]) / 64.0);
    const double angle = 2.0 * opn2_hq_pi * static_cast<double>(phase) / 1024.0;
    const double output = 8192.0 * attenuation * std::sin(angle);
    return std::isfinite(output) ? output : 0.0;
}

inline nuked_opn2_hq_pending_operator opn2_hq_prepare_cycle(
    const ym3438_t& chip,
    nuked_opn2_hq_lift_state& state,
    double lanes[opn2_hq_channel_count][2]) noexcept {
    const Bit32u cycle = chip.cycles;
    const std::size_t bus_channel = opn2_hq_bus_channel(cycle);

    // ChOutput precedes ChGenerate in the reference pipeline.
    if ((cycle & 3u) == 0u) {
        state.locked_output = state.channel_output[bus_channel];
        state.locked_left = chip.pan_l[bus_channel] != 0;
        state.locked_right = chip.pan_r[bus_channel] != 0;
    }

    // Remove the YM2612 sign-leak/DAC-ladder artifact from FM, but preserve the
    // exact authored pan and user mute. DAC remains its own source identity.
    if ((cycle & 3u) == 3u && !(bus_channel == 5u && chip.dacen)) {
        if (!chip.mute[bus_channel]) {
            if (state.locked_left)
                lanes[bus_channel][0] += state.locked_output * 3.0;
            if (state.locked_right)
                lanes[bus_channel][1] += state.locked_output * 3.0;
        }
    }

    const std::size_t channel = static_cast<std::size_t>(chip.channel);
    const std::size_t consumed_slot = static_cast<std::size_t>((cycle + 18u) % 24u);
    const std::size_t op = consumed_slot / 6u;
    if (channel < opn2_hq_channel_count && op < 4u) {
        double next = state.channel_accumulator[channel];
        if (op == 0u) {
            state.channel_output[channel] = state.channel_accumulator[channel];
            next = 0.0;
        }
        const std::size_t algorithm = static_cast<std::size_t>(chip.connect[channel] & 0x07u);
        if (opn2_carrier_table[op][algorithm])
            next += state.operator_output[consumed_slot] / 32.0;
        state.channel_accumulator[channel] = next;
    }

    const std::size_t generated_slot = static_cast<std::size_t>((cycle + 19u) % 24u);
    return {generated_slot, opn2_hq_operator_from_exact_state(chip, generated_slot)};
}

inline void opn2_hq_finish_cycle(
    const nuked_opn2_hq_pending_operator& pending,
    nuked_opn2_hq_lift_state& state) noexcept {
    if (pending.slot < state.operator_output.size())
        state.operator_output[pending.slot] = pending.output;
}

} // namespace foobar_vgm::genesis
