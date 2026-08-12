#include "genesis_state.h"

namespace gameaudio::vgm {

namespace {

constexpr bool payload_has(const command_event& event, std::uint32_t count) noexcept {
    return event.payload != nullptr && event.payload_size >= count;
}

constexpr int ym_channel_from_key_code(std::uint8_t code) noexcept {
    code &= 0x07;
    if (code == 0x03 || code == 0x07)
        return -1;
    return static_cast<int>(code & 0x03) + ((code & 0x04) ? 3 : 0);
}

constexpr int ym_channel_from_port_register(std::uint8_t port, std::uint8_t reg) noexcept {
    const std::uint8_t local = reg & 0x03;
    if (local >= 3 || port >= 2)
        return -1;
    return static_cast<int>(port) * 3 + local;
}

constexpr int ym_operator_from_register(std::uint8_t reg) noexcept {
    // Yamaha's register order is OP1, OP3, OP2, OP4 at +0,+4,+8,+C.
    constexpr int register_slot_to_operator[4] = {0, 2, 1, 3};
    return register_slot_to_operator[(reg >> 2) & 0x03];
}

void write_ym_operator(ym2612_state& state, std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept {
    if (reg < 0x30 || reg > 0x9F)
        return;
    const int channel = ym_channel_from_port_register(port, reg);
    if (channel < 0)
        return;

    const int op_index = ym_operator_from_register(reg);
    auto& op = state.channels[static_cast<std::size_t>(channel)].operators[static_cast<std::size_t>(op_index)];

    switch (reg & 0xF0) {
    case 0x30:
        op.multiple = static_cast<std::uint8_t>(data & 0x0F);
        op.detune = static_cast<std::uint8_t>((data >> 4) & 0x07);
        break;
    case 0x40:
        op.total_level = static_cast<std::uint8_t>(data & 0x7F);
        break;
    case 0x50:
        op.attack_rate = static_cast<std::uint8_t>(data & 0x1F);
        op.key_scale = static_cast<std::uint8_t>((data >> 6) & 0x03);
        break;
    case 0x60:
        op.decay_rate = static_cast<std::uint8_t>(data & 0x1F);
        op.amplitude_modulation = (data & 0x80) != 0;
        break;
    case 0x70:
        op.sustain_rate = static_cast<std::uint8_t>(data & 0x1F);
        break;
    case 0x80:
        op.release_rate = static_cast<std::uint8_t>(data & 0x0F);
        op.sustain_level = static_cast<std::uint8_t>((data >> 4) & 0x0F);
        break;
    case 0x90:
        op.ssg_eg = static_cast<std::uint8_t>(data & 0x0F);
        break;
    default:
        break;
    }
}

} // namespace

void genesis_state::reset() noexcept {
    *this = genesis_state{};
}

void genesis_state::observe(const command_event& event) noexcept {
    if (event.kind == command_event_kind::reset) {
        reset();
        return;
    }

    if (event.kind == command_event_kind::ym2612_dac) {
        if (payload_has(event, 1)) {
            ym2612_[0].last_dac_sample = event.payload[0];
            ++ym2612_[0].resolved_stream_dac_write_count;
        }
        return;
    }

    ++observed_commands_;

    switch (event.command) {
    case 0x50:
        if (payload_has(event, 1)) {
            write_psg(psg_[0], event.payload[0]);
            ++psg_writes_;
        }
        return;
    case 0x30:
        if (payload_has(event, 1)) {
            write_psg(psg_[1], event.payload[0]);
            ++psg_writes_;
        }
        return;
    case 0x4F:
        if (payload_has(event, 1))
            psg_[0].stereo_mask = event.payload[0];
        return;
    case 0x52:
    case 0x53:
        if (payload_has(event, 2)) {
            write_ym2612(ym2612_[0], static_cast<std::uint8_t>(event.command - 0x52), event.payload[0], event.payload[1]);
            ++ym2612_writes_;
        }
        return;
    case 0xA2:
    case 0xA3:
        if (payload_has(event, 2)) {
            write_ym2612(ym2612_[1], static_cast<std::uint8_t>(event.command - 0xA2), event.payload[0], event.payload[1]);
            ++ym2612_writes_;
        }
        return;
    default:
        break;
    }

    if ((event.command & 0xF0) == 0x80)
        ++ym2612_[0].stream_dac_step_count;
}

void genesis_state::write_ym2612(ym2612_state& state, std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept {
    if (port >= state.registers.size())
        return;

    state.registers[port][reg] = data;
    write_ym_operator(state, port, reg, data);

    if (port == 0) {
        switch (reg) {
        case 0x22:
            state.lfo_enabled = (data & 0x08) != 0;
            state.lfo_frequency = static_cast<std::uint8_t>(data & 0x07);
            return;
        case 0x27:
            state.channel3_mode = static_cast<std::uint8_t>((data >> 6) & 0x03);
            state.csm_enabled = state.channel3_mode == 2;
            return;
        case 0x2A:
            state.last_dac_sample = data;
            ++state.direct_dac_write_count;
            return;
        case 0x2B:
            state.dac_enabled = (data & 0x80) != 0;
            return;
        case 0x28: {
            const int channel = ym_channel_from_key_code(data);
            if (channel >= 0) {
                auto& ch = state.channels[static_cast<std::size_t>(channel)];
                ch.operator_key_mask = static_cast<std::uint8_t>((data >> 4) & 0x0F);
                ch.key_on = ch.operator_key_mask != 0;
            }
            return;
        }
        default:
            break;
        }
    }

    // A4..A6 latch the high F-number/block byte. The value is committed only
    // when the matching A0..A2 low-byte write arrives.
    if (reg >= 0xA4 && reg <= 0xA6) {
        state.fnum_high_latch = data;
        return;
    }
    if (reg >= 0xA0 && reg <= 0xA2) {
        const int channel = ym_channel_from_port_register(port, reg);
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.fnum = static_cast<std::uint16_t>(data | ((state.fnum_high_latch & 0x07u) << 8));
            ch.block = static_cast<std::uint8_t>((state.fnum_high_latch >> 3) & 0x07u);
        }
        return;
    }

    // Channel 3 special-mode operator frequencies use the same latch/commit
    // pattern through AC..AE and A8..AA on port 0.
    if (port == 0 && reg >= 0xAC && reg <= 0xAE) {
        state.ch3_fnum_high_latch = data;
        return;
    }
    if (port == 0 && reg >= 0xA8 && reg <= 0xAA) {
        const std::size_t index = static_cast<std::size_t>(reg - 0xA8);
        state.ch3_fnum[index] = static_cast<std::uint16_t>(data | ((state.ch3_fnum_high_latch & 0x07u) << 8));
        state.ch3_block[index] = static_cast<std::uint8_t>((state.ch3_fnum_high_latch >> 3) & 0x07u);
        return;
    }

    if (reg >= 0xB0 && reg <= 0xB2) {
        const int channel = ym_channel_from_port_register(port, reg);
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.algorithm = static_cast<std::uint8_t>(data & 0x07u);
            ch.feedback = static_cast<std::uint8_t>((data >> 3) & 0x07u);
        }
        return;
    }

    if (reg >= 0xB4 && reg <= 0xB6) {
        const int channel = ym_channel_from_port_register(port, static_cast<std::uint8_t>(reg - 4));
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.pan_left = (data & 0x80u) != 0;
            ch.pan_right = (data & 0x40u) != 0;
            ch.ams = static_cast<std::uint8_t>((data >> 4) & 0x03u);
            ch.fms = static_cast<std::uint8_t>(data & 0x07u);
        }
    }
}

void genesis_state::write_psg(sn76489_state& state, std::uint8_t data) noexcept {
    if (data & 0x80u) {
        state.latched_channel = static_cast<std::uint8_t>((data >> 5) & 0x03u);
        state.latched_volume = (data & 0x10u) != 0;

        if (state.latched_volume) {
            state.channels[state.latched_channel].attenuation = static_cast<std::uint8_t>(data & 0x0Fu);
        } else if (state.latched_channel < 3) {
            auto& period = state.channels[state.latched_channel].tone_period;
            period = static_cast<std::uint16_t>((period & 0x03F0u) | (data & 0x0Fu));
        } else {
            state.noise_control = static_cast<std::uint8_t>(data & 0x07u);
        }
        return;
    }

    if (state.latched_volume) {
        state.channels[state.latched_channel].attenuation = static_cast<std::uint8_t>(data & 0x0Fu);
    } else if (state.latched_channel < 3) {
        auto& period = state.channels[state.latched_channel].tone_period;
        period = static_cast<std::uint16_t>((period & 0x000Fu) | ((data & 0x3Fu) << 4));
    } else {
        state.noise_control = static_cast<std::uint8_t>(data & 0x07u);
    }
}

} // namespace gameaudio::vgm
