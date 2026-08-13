#pragma once

#include "yamaha_opl_family.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

constexpr std::optional<std::uint8_t> opl_channel_from_port_register(
    const opl_family_traits traits,
    const std::uint8_t port,
    const std::uint8_t reg,
    const std::uint8_t base) noexcept {
    if (port >= traits.fm_ports || reg < base || reg > static_cast<std::uint8_t>(base + 8u))
        return std::nullopt;

    const std::uint8_t local = static_cast<std::uint8_t>(reg - base);
    const std::uint8_t channel = static_cast<std::uint8_t>(port * 9u + local);
    if (channel >= traits.fm_channels)
        return std::nullopt;
    return channel;
}

constexpr bool opl_frequency_low_register(const std::uint8_t reg) noexcept {
    return reg >= 0xA0u && reg <= 0xA8u;
}

constexpr bool opl_frequency_high_key_register(const std::uint8_t reg) noexcept {
    return reg >= 0xB0u && reg <= 0xB8u;
}

constexpr bool opl_connection_feedback_register(const std::uint8_t reg) noexcept {
    return reg >= 0xC0u && reg <= 0xC8u;
}

struct opl_block_fnum {
    std::uint16_t fnum = 0;
    std::uint8_t block = 0;
    bool key_on = false;
};

constexpr opl_block_fnum decode_opl_block_fnum(
    const std::uint8_t low,
    const std::uint8_t high_key) noexcept {
    return opl_block_fnum{
        static_cast<std::uint16_t>(low | ((high_key & 0x03u) << 8)),
        static_cast<std::uint8_t>((high_key >> 2) & 0x07u),
        (high_key & 0x20u) != 0,
    };
}

struct opl_connection_feedback {
    std::uint8_t connection = 0;
    std::uint8_t feedback = 0;
};

// OPL uses a one-bit connection selector and feedback in bits 1-3. This is
// intentionally separate from the OPN/OPM 3-bit algorithm + 3-bit feedback
// encoding even though all three are Yamaha FM families.
constexpr opl_connection_feedback decode_opl_connection_feedback(
    const std::uint8_t data) noexcept {
    return opl_connection_feedback{
        static_cast<std::uint8_t>(data & 0x01u),
        static_cast<std::uint8_t>((data >> 1) & 0x07u),
    };
}

// OPL3's second register bank contains dynamic four-operator pairing and new
// mode controls at absolute registers 0x104 and 0x105. In VGM, bank 1 is the
// 0x5F command and the register byte remains 0x04/0x05.
constexpr bool opl3_four_op_enable_register(
    const std::uint8_t port,
    const std::uint8_t reg) noexcept {
    return port == 1u && reg == 0x04u;
}

constexpr bool opl3_new_mode_register(
    const std::uint8_t port,
    const std::uint8_t reg) noexcept {
    return port == 1u && reg == 0x05u;
}

constexpr std::uint8_t opl3_four_op_pair_mask(const std::uint8_t data) noexcept {
    return static_cast<std::uint8_t>(data & 0x3Fu);
}

constexpr bool opl3_new_mode_enabled(const std::uint8_t data) noexcept {
    return (data & 0x01u) != 0;
}

// In OPL3 new mode, C0-C8 bits 4-7 gate the four physical output buses.
constexpr std::uint8_t opl3_output_mask(const std::uint8_t data) noexcept {
    return static_cast<std::uint8_t>((data >> 4) & 0x0Fu);
}

struct opl_rhythm_state {
    bool rhythm_enabled = false;
    std::uint8_t percussion_key_mask = 0;
};

constexpr opl_rhythm_state decode_opl_rhythm_state(const std::uint8_t data) noexcept {
    return opl_rhythm_state{
        (data & 0x20u) != 0,
        static_cast<std::uint8_t>(data & 0x1Fu),
    };
}

} // namespace gameaudio::vgm
