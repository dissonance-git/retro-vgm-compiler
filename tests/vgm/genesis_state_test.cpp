#include "../../components/vgm/enhancement/genesis_state.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

using gameaudio::vgm::command_event;
using gameaudio::vgm::genesis_state;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static command_event event(std::uint8_t command, std::initializer_list<std::uint8_t> payload = {}) {
    static std::vector<std::uint8_t> storage;
    storage.assign(payload.begin(), payload.end());
    return command_event{gameaudio::vgm::command_event_kind::command, 0, 0, command, storage.empty() ? nullptr : storage.data(), static_cast<std::uint32_t>(storage.size())};
}

int main() {
    genesis_state state;

    // YM2612 pitch follows the real A4 latch -> A0 commit sequence.
    state.observe(event(0x52, {0xA4, 0x22}));
    CHECK(state.ym2612().channels[0].fnum == 0);
    state.observe(event(0x52, {0xA0, 0x34}));
    CHECK(state.ym2612().channels[0].fnum == 0x234);
    CHECK(state.ym2612().channels[0].block == 4);

    // A later A4 write alone must not mutate an already committed pitch.
    state.observe(event(0x52, {0xA4, 0x19}));
    CHECK(state.ym2612().channels[0].fnum == 0x234);
    CHECK(state.ym2612().channels[0].block == 4);

    // Full operator state. Register order is OP1, OP3, OP2, OP4.
    state.observe(event(0x52, {0x30, 0x71})); // OP1 DT/MUL
    state.observe(event(0x52, {0x38, 0x2F})); // OP2 DT/MUL
    state.observe(event(0x52, {0x34, 0x43})); // OP3 DT/MUL
    state.observe(event(0x52, {0x3C, 0x64})); // OP4 DT/MUL
    CHECK(state.ym2612().channels[0].operators[0].detune == 7);
    CHECK(state.ym2612().channels[0].operators[0].multiple == 1);
    CHECK(state.ym2612().channels[0].operators[1].detune == 2);
    CHECK(state.ym2612().channels[0].operators[1].multiple == 15);
    CHECK(state.ym2612().channels[0].operators[2].detune == 4);
    CHECK(state.ym2612().channels[0].operators[2].multiple == 3);
    CHECK(state.ym2612().channels[0].operators[3].detune == 6);
    CHECK(state.ym2612().channels[0].operators[3].multiple == 4);

    state.observe(event(0x52, {0x40, 0x55}));
    state.observe(event(0x52, {0x50, 0xDF}));
    state.observe(event(0x52, {0x60, 0x9A}));
    state.observe(event(0x52, {0x70, 0x13}));
    state.observe(event(0x52, {0x80, 0xA7}));
    state.observe(event(0x52, {0x90, 0x0D}));
    const auto& op1 = state.ym2612().channels[0].operators[0];
    CHECK(op1.total_level == 0x55);
    CHECK(op1.key_scale == 3);
    CHECK(op1.attack_rate == 0x1F);
    CHECK(op1.amplitude_modulation);
    CHECK(op1.decay_rate == 0x1A);
    CHECK(op1.sustain_rate == 0x13);
    CHECK(op1.sustain_level == 0x0A);
    CHECK(op1.release_rate == 0x07);
    CHECK(op1.ssg_eg == 0x0D);

    state.observe(event(0x52, {0xB0, 0x2D}));
    CHECK(state.ym2612().channels[0].algorithm == 5);
    CHECK(state.ym2612().channels[0].feedback == 5);

    state.observe(event(0x52, {0xB4, 0xF7}));
    CHECK(state.ym2612().channels[0].pan_left);
    CHECK(state.ym2612().channels[0].pan_right);
    CHECK(state.ym2612().channels[0].ams == 3);
    CHECK(state.ym2612().channels[0].fms == 7);

    // Global LFO and channel-3 mode are source truth too.
    state.observe(event(0x52, {0x22, 0x0D}));
    state.observe(event(0x52, {0x27, 0x80}));
    CHECK(state.ym2612().lfo_enabled);
    CHECK(state.ym2612().lfo_frequency == 5);
    CHECK(state.ym2612().channel3_mode == 2);
    CHECK(state.ym2612().csm_enabled);

    // Channel-3 special operator frequency latch/commit.
    state.observe(event(0x52, {0xAC, 0x1B}));
    state.observe(event(0x52, {0xA8, 0x66}));
    CHECK(state.ym2612().ch3_fnum[0] == 0x366);
    CHECK(state.ym2612().ch3_block[0] == 3);

    state.observe(event(0x52, {0x28, 0xF5}));
    CHECK(state.ym2612().channels[4].key_on);
    CHECK(state.ym2612().channels[4].operator_key_mask == 0x0F);
    state.observe(event(0x52, {0x28, 0x05}));
    CHECK(!state.ym2612().channels[4].key_on);

    state.observe(event(0x52, {0x2B, 0x80}));
    state.observe(event(0x52, {0x2A, 0x6C}));
    state.observe(event(0x8F));
    const std::uint8_t resolved_dac = 0x93;
    state.observe(command_event{gameaudio::vgm::command_event_kind::ym2612_dac, 0, 0, 0x8F, &resolved_dac, 1});
    CHECK(state.ym2612().dac_enabled);
    CHECK(state.ym2612().last_dac_sample == 0x93);
    CHECK(state.ym2612().direct_dac_write_count == 1);
    CHECK(state.ym2612().stream_dac_step_count == 1);
    CHECK(state.ym2612().resolved_stream_dac_write_count == 1);

    state.observe(event(0x50, {0x8A}));
    state.observe(event(0x50, {0x15}));
    CHECK(state.psg().channels[0].tone_period == 0x15A);

    state.observe(event(0x50, {0xBC}));
    CHECK(state.psg().channels[1].attenuation == 12);

    state.observe(event(0xA3, {0xB4, 0x80}));
    CHECK(state.ym2612(1).channels[3].pan_left);
    CHECK(!state.ym2612(1).channels[3].pan_right);
    CHECK(state.ym2612(0).channels[3].pan_right);

    CHECK(state.ym2612_writes() > 20);
    CHECK(state.psg_writes() == 3);

    state.observe(command_event{gameaudio::vgm::command_event_kind::reset});
    CHECK(state.observed_commands() == 0);
    CHECK(state.ym2612().channels[0].fnum == 0);
    CHECK(state.ym2612().channels[0].operators[0].multiple == 0);
    CHECK(state.psg().channels[0].attenuation == 0x0F);

    return 0;
}
