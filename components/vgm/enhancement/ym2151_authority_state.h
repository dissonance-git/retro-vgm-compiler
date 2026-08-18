#pragma once

#include "yamaha_opm_register.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Register-derived OPM authority. This is deliberately not an emulator state:
// phase accumulators, envelope counters, timer phase, feedback memory, LFO phase
// and noise LFSR evolve with time inside the renderer. The shadow records the
// authored controls a candidate renderer must obey exactly while evolving those
// hidden states itself.
struct ym2151_operator_authority {
    std::uint8_t dt1 = 0;
    std::uint8_t multiple = 0;
    std::uint8_t total_level = 0;
    std::uint8_t key_scale = 0;
    std::uint8_t attack_rate = 0;
    bool amplitude_modulation = false;
    std::uint8_t decay1_rate = 0;
    std::uint8_t dt2 = 0;
    std::uint8_t decay2_rate = 0;
    std::uint8_t sustain_level = 0;
    std::uint8_t release_rate = 0;
};

struct ym2151_channel_authority {
    bool route_left = false;
    bool route_right = false;
    std::uint8_t feedback = 0;
    std::uint8_t algorithm = 0;
    std::uint8_t key_code = 0;
    std::uint8_t key_fraction = 0;
    std::uint8_t pms = 0;
    std::uint8_t ams = 0;
    std::uint8_t key_mask = 0;
    std::array<ym2151_operator_authority, 4> operators{};
};

struct ym2151_global_authority {
    std::uint8_t test = 0;
    bool noise_enabled = false;
    std::uint8_t noise_period = 0;
    std::uint16_t timer_a = 0;
    std::uint8_t timer_b = 0;
    std::uint8_t timer_control = 0;
    bool csm_enabled = false;
    std::uint8_t lfo_frequency = 0;
    std::uint8_t pmd = 0;
    std::uint8_t amd = 0;
    std::uint8_t lfo_waveform = 0;
    std::uint8_t ct = 0;
};

class ym2151_authority_state {
public:
    static constexpr std::size_t channel_count = 8;
    static constexpr std::size_t operator_count = 4;

    ym2151_authority_state() noexcept { reset(); }

    void reset() noexcept {
        registers_.fill(0);
        channels_ = {};
        global_ = {};
        write_count_ = 0;
    }

    void apply_register(std::uint8_t reg, std::uint8_t value) noexcept {
        registers_[reg] = value;
        ++write_count_;

        if (reg < 0x20u) {
            apply_global(reg, value);
            return;
        }

        if (reg < 0x40u) {
            const std::size_t channel = static_cast<std::size_t>(reg & 0x07u);
            auto& state = channels_[channel];
            switch (reg & 0x18u) {
            case 0x00u: {
                const auto af = opm_algorithm_feedback(value);
                const auto route = decode_opm_stereo_route(value);
                state.algorithm = af.algorithm;
                state.feedback = af.feedback;
                state.route_left = route.left;
                state.route_right = route.right;
                break;
            }
            case 0x08u:
                state.key_code = static_cast<std::uint8_t>(value & 0x7fu);
                break;
            case 0x10u:
                state.key_fraction = static_cast<std::uint8_t>((value >> 2) & 0x3fu);
                break;
            case 0x18u:
                state.pms = static_cast<std::uint8_t>((value >> 4) & 0x07u);
                state.ams = static_cast<std::uint8_t>(value & 0x03u);
                break;
            default:
                break;
            }
            return;
        }

        const std::size_t channel = static_cast<std::size_t>(opm_operator_channel(reg));
        const std::size_t op_index = static_cast<std::size_t>(opm_operator_from_register(reg));
        auto& op = channels_[channel].operators[op_index];
        switch (reg & 0xe0u) {
        case 0x40u:
            op.dt1 = static_cast<std::uint8_t>((value >> 4) & 0x07u);
            op.multiple = static_cast<std::uint8_t>(value & 0x0fu);
            break;
        case 0x60u:
            op.total_level = static_cast<std::uint8_t>(value & 0x7fu);
            break;
        case 0x80u:
            op.key_scale = static_cast<std::uint8_t>((value >> 6) & 0x03u);
            op.attack_rate = static_cast<std::uint8_t>(value & 0x1fu);
            break;
        case 0xa0u:
            op.amplitude_modulation = (value & 0x80u) != 0u;
            op.decay1_rate = static_cast<std::uint8_t>(value & 0x1fu);
            break;
        case 0xc0u:
            op.dt2 = static_cast<std::uint8_t>((value >> 6) & 0x03u);
            op.decay2_rate = static_cast<std::uint8_t>(value & 0x1fu);
            break;
        case 0xe0u:
            op.sustain_level = static_cast<std::uint8_t>((value >> 4) & 0x0fu);
            op.release_rate = static_cast<std::uint8_t>(value & 0x0fu);
            break;
        default:
            break;
        }
    }

    const ym2151_channel_authority& channel(std::size_t index) const noexcept {
        return channels_[index < channel_count ? index : 0u];
    }

    const ym2151_global_authority& global() const noexcept { return global_; }
    std::uint8_t raw_register(std::uint8_t reg) const noexcept { return registers_[reg]; }
    std::uint64_t write_count() const noexcept { return write_count_; }

private:
    void apply_global(std::uint8_t reg, std::uint8_t value) noexcept {
        switch (reg) {
        case 0x01u:
            global_.test = value;
            break;
        case 0x08u: {
            const std::size_t channel = static_cast<std::size_t>(opm_key_channel(value));
            channels_[channel].key_mask = opm_key_operator_mask(value);
            break;
        }
        case 0x0fu:
            global_.noise_enabled = (value & 0x80u) != 0u;
            global_.noise_period = static_cast<std::uint8_t>(value & 0x1fu);
            break;
        case 0x10u:
            global_.timer_a = static_cast<std::uint16_t>(
                (global_.timer_a & 0x0003u) | (static_cast<std::uint16_t>(value) << 2u));
            break;
        case 0x11u:
            global_.timer_a = static_cast<std::uint16_t>(
                (global_.timer_a & 0x03fcu) | static_cast<std::uint16_t>(value & 0x03u));
            break;
        case 0x12u:
            global_.timer_b = value;
            break;
        case 0x14u:
            global_.timer_control = value;
            global_.csm_enabled = (value & 0x80u) != 0u;
            break;
        case 0x18u:
            global_.lfo_frequency = value;
            break;
        case 0x19u:
            if ((value & 0x80u) != 0u)
                global_.pmd = static_cast<std::uint8_t>(value & 0x7fu);
            else
                global_.amd = static_cast<std::uint8_t>(value & 0x7fu);
            break;
        case 0x1bu:
            global_.ct = static_cast<std::uint8_t>(value >> 6);
            global_.lfo_waveform = static_cast<std::uint8_t>(value & 0x03u);
            break;
        default:
            break;
        }
    }

    std::array<std::uint8_t, 256> registers_{};
    std::array<ym2151_channel_authority, channel_count> channels_{};
    ym2151_global_authority global_{};
    std::uint64_t write_count_ = 0;
};

} // namespace gameaudio::vgm
