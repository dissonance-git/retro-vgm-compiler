#include "components/vgm/enhancement/ym2151_authority_state.h"

#include <cassert>

int main() {
    using namespace gameaudio::vgm;

    ym2151_authority_state state;
    assert(state.write_count() == 0u);
    assert(!state.channel(0).route_left && !state.channel(0).route_right);

    // Channel 3: both outputs, feedback 5, algorithm 6.
    state.apply_register(0x22u, static_cast<std::uint8_t>(0xC0u | (5u << 3u) | 6u));
    assert(state.channel(2).route_left);
    assert(state.channel(2).route_right);
    assert(state.channel(2).feedback == 5u);
    assert(state.channel(2).algorithm == 6u);

    // KC/KF and PMS/AMS are channel-level OPM authority.
    state.apply_register(0x2Au, 0x5Au);
    state.apply_register(0x32u, 0xFCu);
    state.apply_register(0x3Au, 0x73u);
    assert(state.channel(2).key_code == 0x5Au);
    assert(state.channel(2).key_fraction == 63u);
    assert(state.channel(2).pms == 7u);
    assert(state.channel(2).ams == 3u);

    // Physical register slot 0x48 maps to logical operator 3 (index 2).
    state.apply_register(0x4Au, 0x7Fu); // channel 2, slot 0: DT1/MUL
    state.apply_register(0x6Au, 0x55u); // TL
    state.apply_register(0x8Au, 0xDFu); // KS/AR
    state.apply_register(0xAAu, 0x9Fu); // AM/D1R
    state.apply_register(0xCAu, 0xDFu); // DT2/D2R
    state.apply_register(0xEAu, 0xA9u); // D1L/RR
    const auto& op1 = state.channel(2).operators[0];
    assert(op1.dt1 == 7u && op1.multiple == 15u);
    assert(op1.total_level == 0x55u);
    assert(op1.key_scale == 3u && op1.attack_rate == 31u);
    assert(op1.amplitude_modulation && op1.decay1_rate == 31u);
    assert(op1.dt2 == 3u && op1.decay2_rate == 31u);
    assert(op1.sustain_level == 10u && op1.release_rate == 9u);

    state.apply_register(0x4Au + 0x08u, 0x21u);
    assert(state.channel(2).operators[2].dt1 == 2u);
    assert(state.channel(2).operators[2].multiple == 1u);

    // Key command 0x08 is event-like authority, not one of the channel register
    // banks: bits 3-6 are the four-operator key mask and bits 0-2 the channel.
    state.apply_register(0x08u, static_cast<std::uint8_t>((0x0Bu << 3u) | 2u));
    assert(state.channel(2).key_mask == 0x0Bu);

    state.apply_register(0x0Fu, 0x9Du);
    state.apply_register(0x10u, 0xA5u);
    state.apply_register(0x11u, 0x03u);
    state.apply_register(0x12u, 0x77u);
    state.apply_register(0x14u, 0x83u);
    state.apply_register(0x18u, 0xE7u);
    state.apply_register(0x19u, 0x45u); // AMD
    state.apply_register(0x19u, 0xC2u); // PMD
    state.apply_register(0x1Bu, 0xC3u);

    const auto& global = state.global();
    assert(global.noise_enabled && global.noise_period == 0x1Du);
    assert(global.timer_a == static_cast<std::uint16_t>((0xA5u << 2u) | 3u));
    assert(global.timer_b == 0x77u);
    assert(global.timer_control == 0x83u && global.csm_enabled);
    assert(global.lfo_frequency == 0xE7u);
    assert(global.amd == 0x45u);
    assert(global.pmd == 0x42u);
    assert(global.ct == 3u && global.lfo_waveform == 3u);

    assert(state.raw_register(0x19u) == 0xC2u);
    assert(state.write_count() == 20u);

    state.reset();
    assert(state.write_count() == 0u);
    assert(state.channel(2).key_mask == 0u);
    assert(!state.global().noise_enabled);
    return 0;
}
