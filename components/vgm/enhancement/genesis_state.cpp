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
    // SN76489 / PSG, first and second chip.
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

    // Game Gear stereo mask. Not used by normal Mega Drive playback, but it is
    // source truth when the command is present and costs nothing to preserve.
    case 0x4F:
        if (payload_has(event, 1))
            psg_[0].stereo_mask = event.payload[0];
        return;

    // YM2612 first chip: port 0 / port 1.
    case 0x52:
    case 0x53:
        if (payload_has(event, 2)) {
            write_ym2612(ym2612_[0], static_cast<std::uint8_t>(event.command - 0x52), event.payload[0], event.payload[1]);
            ++ym2612_writes_;
        }
        return;

    // YM2612 second chip: port 0 / port 1.
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

    // 0x80..0x8F write the next YM2612 PCM byte from the active VGM data bank
    // and then delay by N samples. The raw command alone does not expose that
    // byte, so observation records the exact stream activity without inventing
    // sample values. The paired resolved event carries the actual DAC byte.
    if ((event.command & 0xF0) == 0x80) {
        ++ym2612_[0].stream_dac_step_count;
    }
}

void genesis_state::write_ym2612(ym2612_state& state, std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept {
    if (port >= state.registers.size())
        return;

    state.registers[port][reg] = data;

    // DAC data / enable are global YM2612 registers on port 0.
    if (port == 0 && reg == 0x2A) {
        state.last_dac_sample = data;
        ++state.direct_dac_write_count;
        return;
    }
    if (port == 0 && reg == 0x2B) {
        state.dac_enabled = (data & 0x80) != 0;
        return;
    }

    // Key on/off selects a channel independently of the normal port/register
    // channel mapping. Bits 4..7 are the four operator key bits.
    if (port == 0 && reg == 0x28) {
        const int channel = ym_channel_from_key_code(data);
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.operator_key_mask = static_cast<std::uint8_t>((data >> 4) & 0x0F);
            ch.key_on = ch.operator_key_mask != 0;
        }
        return;
    }

    // F-number low byte: A0..A2 per port.
    if (reg >= 0xA0 && reg <= 0xA2) {
        const int channel = ym_channel_from_port_register(port, reg);
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.fnum = static_cast<std::uint16_t>((ch.fnum & 0x0700u) | data);
        }
        return;
    }

    // Block + F-number high bits: A4..A6 per port.
    if (reg >= 0xA4 && reg <= 0xA6) {
        const int channel = ym_channel_from_port_register(port, static_cast<std::uint8_t>(reg - 4));
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.fnum = static_cast<std::uint16_t>((ch.fnum & 0x00FFu) | ((data & 0x07u) << 8));
            ch.block = static_cast<std::uint8_t>((data >> 3) & 0x07u);
        }
        return;
    }

    // Algorithm + feedback: B0..B2 per port.
    if (reg >= 0xB0 && reg <= 0xB2) {
        const int channel = ym_channel_from_port_register(port, reg);
        if (channel >= 0) {
            auto& ch = state.channels[static_cast<std::size_t>(channel)];
            ch.algorithm = static_cast<std::uint8_t>(data & 0x07u);
            ch.feedback = static_cast<std::uint8_t>((data >> 3) & 0x07u);
        }
        return;
    }

    // Stereo routing + AMS/FMS: B4..B6 per port.
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
