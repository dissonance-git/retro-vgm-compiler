#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/cores/ym3438_int.h>
}

// Higher-fidelity six-channel OPN2 descendant synthesized from the one
// authoritative pinned Nuked OPN2 state.
//
// Nuked still owns the musical/control performance: register latency, pitch,
// detune, LFO/PM/AM coordinates, CH3 special mode, CSM, SSG-EG, keying,
// envelope timing, authored panning and mute state. This sidecar keeps the same
// 6 x 4-operator program but raises the numerical ceiling of the FM graph:
//
//   * full 20-bit carrier phase instead of the historical 10-bit log-sine index;
//   * continuous sine/amplitude reconstruction;
//   * floating-point operator feedback/modulation history;
//   * floating-point channel accumulation without the YM2612 9-bit clamp;
//   * no YM2612 DAC-ladder/sign-leak coloration on FM;
//   * no mandatory Mega Drive model-1 output low-pass.
//
// This is deliberately not a DX7/FM-X patch conversion. It does not add
// operators, algorithms, notes or channels. Think of it as the source OPN2
// composition executed on a hypothetical studio-grade Yamaha descendant whose
// controls are still the exact game controls.
namespace foobar_vgm::genesis {

static constexpr std::size_t opn2_hq_channel_count = 6;
static constexpr std::size_t opn2_hq_operator_count = 24;
static constexpr double opn2_hq_pi = 3.141592653589793238462643383279502884;
static constexpr double opn2_hq_phase_modulus = 1048576.0; // 2^20 native phase accumulator

// Exact routing table from the pinned Nuked OPN2 fm_algorithm table. Rows are
// physical operators 1..4. Columns 0..4 select the two modulation inputs and
// previous-operator routes; column 5 marks channel-output carriers.
static constexpr std::uint8_t opn2_hq_algorithm[4][6][8] = {
    {
        {1,1,1,1,1,1,1,1}, // OP1 history 0 -> mod2
        {1,1,1,1,1,1,1,1}, // OP1 history 1 -> mod1
        {0,0,0,0,0,0,0,0}, // OP2 memory -> mod1
        {0,0,0,0,0,0,0,0}, // last operator -> mod2
        {0,0,0,0,0,0,0,0}, // last operator -> mod1
        {0,0,0,0,0,0,0,1}, // carrier
    },
    {
        {0,1,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1},
    },
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {1,0,0,1,1,1,1,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1},
    },
    {
        {0,0,1,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,0,0,0,0},
        {1,1,0,1,1,0,0,0},
        {0,0,1,0,0,0,0,0},
        {1,1,1,1,1,1,1,1},
    },
};

struct nuked_opn2_hq_lift_state {
    // Higher-ceiling equivalents of Nuked's fm_out/fm_mod/fm_op1/fm_op2.
    std::array<double, opn2_hq_operator_count> operator_output{};
    std::array<double, opn2_hq_operator_count> operator_modulation{};
    std::array<std::array<double, 2>, opn2_hq_channel_count> op1_history{};
    std::array<double, opn2_hq_channel_count> op2_memory{};

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
    const nuked_opn2_hq_lift_state& state,
    std::size_t slot) noexcept {
    if (slot >= opn2_hq_operator_count)
        return 0.0;

    // The source chip's phase generator remains authoritative for programmed
    // pitch, detune and LFO/PM. We retain all 20 bits instead of truncating to
    // the ten-bit log-sine ROM address used by the historical output stage.
    double phase_cycles = static_cast<double>(chip.pg_phase[slot])
        / opn2_hq_phase_modulus;

    // fm_mod is expressed in one-q10-phase-step units in Nuked. Recreate that
    // same graph in floating point so a higher-resolution carrier can also be
    // modulated by higher-resolution upstream operators and feedback.
    phase_cycles += state.operator_modulation[slot] / 1024.0;
    phase_cycles -= std::floor(phase_cycles);

    // eg_out is retained from the exact live OPN envelope/LFO-AM state. In the
    // logarithmic hardware path +256 log units halves amplitude; eg_out is
    // shifted by two before that domain, so +64 eg_out units halves amplitude.
    const double attenuation = std::exp2(-static_cast<double>(chip.eg_out[slot]) / 64.0);
    const double angle = 2.0 * opn2_hq_pi * phase_cycles;
    const double output = 8192.0 * attenuation * std::sin(angle);
    return std::isfinite(output) ? output : 0.0;
}

inline void opn2_hq_prepare_modulation(
    const ym3438_t& chip,
    nuked_opn2_hq_lift_state& state) noexcept {
    const std::size_t channel = static_cast<std::size_t>(chip.channel);
    if (channel >= opn2_hq_channel_count)
        return;

    const std::size_t algorithm = static_cast<std::size_t>(chip.connect[channel] & 0x07u);
    const std::size_t prepare_slot = static_cast<std::size_t>((chip.cycles + 6u) % 24u);
    const std::size_t op = prepare_slot / 6u;
    const std::size_t previous_slot = static_cast<std::size_t>((chip.cycles + 18u) % 24u);

    double mod1 = 0.0;
    double mod2 = 0.0;
    if (opn2_hq_algorithm[op][0][algorithm]) mod2 += state.op1_history[channel][0];
    if (opn2_hq_algorithm[op][1][algorithm]) mod1 += state.op1_history[channel][1];
    if (opn2_hq_algorithm[op][2][algorithm]) mod1 += state.op2_memory[channel];
    if (opn2_hq_algorithm[op][3][algorithm]) mod2 += state.operator_output[previous_slot];
    if (opn2_hq_algorithm[op][4][algorithm]) mod1 += state.operator_output[previous_slot];

    double modulation = mod1 + mod2;
    if (op == 0u) {
        const std::uint8_t feedback = static_cast<std::uint8_t>(chip.fb[channel] & 0x07u);
        if (feedback == 0u)
            modulation = 0.0;
        else
            modulation /= static_cast<double>(1u << (10u - feedback));
    } else {
        modulation *= 0.5;
    }
    state.operator_modulation[prepare_slot] = std::isfinite(modulation) ? modulation : 0.0;

    // Match the reference OPN2_FMPrepare delayed history updates, but retain the
    // values as doubles instead of quantized fm_out integers.
    const std::size_t history_op = previous_slot / 6u;
    if (history_op == 0u) {
        state.op1_history[channel][1] = state.op1_history[channel][0];
        state.op1_history[channel][0] = state.operator_output[previous_slot];
    }
    if (history_op == 2u)
        state.op2_memory[channel] = state.operator_output[previous_slot];
}

inline nuked_opn2_hq_pending_operator opn2_hq_prepare_cycle(
    const ym3438_t& chip,
    nuked_opn2_hq_lift_state& state,
    double lanes[opn2_hq_channel_count][2]) noexcept {
    const Bit32u cycle = chip.cycles;
    const std::size_t bus_channel = opn2_hq_bus_channel(cycle);

    // Recreate FMPrepare before the operator generated on this cycle is read.
    // The source chip still supplies every control coordinate; only the
    // synthesis arithmetic of the modulation graph is lifted.
    opn2_hq_prepare_modulation(chip, state);

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
        if (opn2_hq_algorithm[op][5][algorithm])
            next += state.operator_output[consumed_slot] / 32.0;
        state.channel_accumulator[channel] = std::isfinite(next) ? next : 0.0;
    }

    // OPN2_FMGenerate runs before the reference phase/envelope update in this
    // cycle, so the pre-clock state is exactly the generated slot's source
    // control state. The HQ sidecar differs only in arithmetic precision.
    const std::size_t generated_slot = static_cast<std::size_t>((cycle + 19u) % 24u);
    return {
        generated_slot,
        opn2_hq_operator_from_exact_state(chip, state, generated_slot),
    };
}

inline void opn2_hq_finish_cycle(
    const nuked_opn2_hq_pending_operator& pending,
    nuked_opn2_hq_lift_state& state) noexcept {
    if (pending.slot < state.operator_output.size())
        state.operator_output[pending.slot] = pending.output;
}

} // namespace foobar_vgm::genesis
