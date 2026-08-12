#include "../../components/vgm/enhancement/genesis_state.h"

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <vector>

using gameaudio::vgm::command_event;
using gameaudio::vgm::genesis_state;

static command_event event(std::uint8_t command, std::initializer_list<std::uint8_t> payload = {}) {
    static std::vector<std::uint8_t> storage;
    storage.assign(payload.begin(), payload.end());
    return command_event{0, 0, command, storage.empty() ? nullptr : storage.data(), static_cast<std::uint32_t>(storage.size())};
}

int main() {
    genesis_state state;

    // YM2612 channel 1 pitch: FNUM 0x234, block 4.
    state.observe(event(0x52, {0xA0, 0x34}));
    state.observe(event(0x52, {0xA4, 0x22}));
    assert(state.ym2612().channels[0].fnum == 0x234);
    assert(state.ym2612().channels[0].block == 4);

    // Algorithm 5, feedback 5.
    state.observe(event(0x52, {0xB0, 0x2D}));
    assert(state.ym2612().channels[0].algorithm == 5);
    assert(state.ym2612().channels[0].feedback == 5);

    // Stereo routing and modulation state.
    state.observe(event(0x52, {0xB4, 0xF7}));
    assert(state.ym2612().channels[0].pan_left);
    assert(state.ym2612().channels[0].pan_right);
    assert(state.ym2612().channels[0].ams == 3);
    assert(state.ym2612().channels[0].fms == 7);

    // Key on all operators on YM channel 5 (zero-based index 4).
    state.observe(event(0x52, {0x28, 0xF5}));
    assert(state.ym2612().channels[4].key_on);
    assert(state.ym2612().channels[4].operator_key_mask == 0x0F);
    state.observe(event(0x52, {0x28, 0x05}));
    assert(!state.ym2612().channels[4].key_on);

    // Direct DAC state and implicit VGM DAC stream activity.
    state.observe(event(0x52, {0x2B, 0x80}));
    state.observe(event(0x52, {0x2A, 0x6C}));
    state.observe(event(0x8F));
    assert(state.ym2612().dac_enabled);
    assert(state.ym2612().last_dac_sample == 0x6C);
    assert(state.ym2612().direct_dac_write_count == 1);
    assert(state.ym2612().stream_dac_step_count == 1);

    // PSG tone channel 0: 10-bit period 0x15A.
    state.observe(event(0x50, {0x8A}));
    state.observe(event(0x50, {0x15}));
    assert(state.psg().channels[0].tone_period == 0x15A);

    // PSG channel 1 volume attenuation = 12.
    state.observe(event(0x50, {0xBC}));
    assert(state.psg().channels[1].attenuation == 12);

    // A second YM2612 instance remains independent.
    state.observe(event(0xA3, {0xB4, 0x80}));
    assert(state.ym2612(1).channels[3].pan_left);
    assert(!state.ym2612(1).channels[3].pan_right);
    assert(state.ym2612(0).channels[3].pan_right);

    assert(state.observed_commands() == 13);
    assert(state.ym2612_writes() == 9);
    assert(state.psg_writes() == 3);

    state.reset();
    assert(state.observed_commands() == 0);
    assert(state.ym2612().channels[0].fnum == 0);
    assert(state.psg().channels[0].attenuation == 0x0F);

    return 0;
}
