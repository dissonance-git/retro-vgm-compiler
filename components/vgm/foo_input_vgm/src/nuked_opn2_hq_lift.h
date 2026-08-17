#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// High-fidelity state lift for the pinned Nuked OPN2 core.
//
// This is deliberately narrower than a second FM emulator. The authoritative
// Nuked engine still owns every musical/control behavior: register latency,
// FNUM, detune, multipliers, LFO/PM/AM, CH3 special mode, CSM, SSG-EG, keying,
// envelope timing and the quantized modulation history. We read that exact live
// state and relax only the final carrier reconstruction/channel ceiling:
//
//   exact q10 modulated phase + exact OPN envelope attenuation
//       -> continuous sine
//       -> floating carrier sum with the exact OPN algorithm topology
//       -> no 9-bit channel accumulator clamp
//       -> no YM2612 sign-leak/DAC-ladder artifact on FM output
//
// The result is an identity-conservative first "better Yamaha" rung. A deeper
// all-floating modulation graph can sit above it experimentally, but this lift
// is safe across far more source semantics because the hardware model remains
// the control-state teacher.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_hq_channel_count = 6;
static constexpr std::size_t opn2_hq_operator_count = 24;
static constexpr double opn2_hq_pi = 3.141592653589793238462643383279502884;

// Exact carrier-connect table from pinned Nuked OPN2's fm_algorithm[][5][].
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

    // OPN2_FMGenerate uses this exact q10 phase coordinate before its logarithmic
    // sine/exp ROM approximation. Keep the exact hardware modulation history but
    // evaluate the carrier continuously.
    const Bit32u phase = static_cast<Bit32u>(
        chip.fm_mod[slot] + (chip.pg_phase[slot] >> 10)) & 0x03ffu;

    // In Nuked, eg_out is added as eg_out<<2 to the log amplitude. A +256
    // increment in that log coordinate halves amplitude, so 64 eg_out steps are
    // one amplitude octave. This continuous form preserves exact EG state while
    // removing logsin/exp table quantization.
    const double attenuation = std::exp2(-static_cast<double>(chip.eg_out[slot]) / 64.0);
    const double angle = 2.0 * opn2_hq_pi * static_cast<double>(phase) / 1024.0;
    const double output = 8192.0 * attenuation * std::sin(angle);
    return std::isfinite(output) ? output : 0.0;
}

inline void opn2_hq_prepare_cycle(
    const ym3438_t& chip,
    nuked_opn2_hq_lift_state& state,
    double lanes[opn2_hq_channel_count][2]) noexcept {
    const Bit32u cycle = chip.cycles;

    // ChOutput occurs before ChGenerate in Nuked. Mirror its lock at the first
    // cycle of each four-cycle bus group using the same channel permutation.
    const std::size_t bus_channel = opn2_hq_bus_channel(cycle);
    if ((cycle & 3u) == 0u) {
        state.locked_output = state.channel_output[bus_channel];
        state.locked_left = chip.pan_l[bus_channel] != 0;
        state.locked_right = chip.pan_r[bus_channel] != 0;
    }

    // Enhanced FM intentionally excludes the YM2612 output-ladder sign leakage.
    // Emit only the real bus slot. Channel 6 FM is silent while DAC is enabled,
    // matching the source identity split used by the exact lane capture.
    if ((cycle & 3u) == 3u && !(bus_channel == 5u && chip.dacen)) {
        if (!chip.mute[bus_channel]) {
            if (state.locked_left)
                lanes[bus_channel][0] += state.locked_output * 3.0;
            if (state.locked_right)
                lanes[bus_channel][1] += state.locked_output * 3.0;
        }
    }

    // ChGenerate consumes the operator produced on the preceding pipeline cycle.
    const std::size_t channel = static_cast<std::size_t>(chip.channel);
    const std::size_t slot = static_cast<std::size_t>((cycle + 18u) % 24u);
    const std::size_t op = slot / 6u;
    if (channel < opn2_hq_channel_count && op < 4u) {
        double next = state.channel_accumulator[channel];
        if (op == 0u) {
            state.channel_output[channel] = state.channel_accumulator[channel];
            next = 0.0;
        }
        const std::size_t algorithm = static_cast<std::size_t>(chip.connect[channel] & 0x07u);
        if (opn2_carrier_table[op][algorithm])
            next += state.operator_output[slot] / 32.0;
        state.channel_accumulator[channel] = next;
    }

    // FMGenerate, later in this same Nuked cycle, produces slot cycle+19 from
    // the pre-cycle phase/modulation/envelope state. Calculate the continuous
    // counterpart now, then publish it after the authoritative clock executes.
}

inline void opn2_hq_finish_cycle(
    const ym3438_t& pre_clock_state,
    nuked_opn2_hq_lift_state& state) noexcept {
    const std::size_t generated_slot = static_cast<std::size_t>(
        (pre_clock_state.cycles + 19u) % 24u);
    state.operator_output[generated_slot] =
        opn2_hq_operator_from_exact_state(pre_clock_state, generated_slot);
}

} // namespace foobar_vgm::genesis
