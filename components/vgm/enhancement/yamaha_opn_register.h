#pragma once

#include "yamaha_four_op_fm.h"
#include "yamaha_opn_family.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// These helpers describe only register geometry shared by the OPN FM family.
// They do not imply that every register has the same audible meaning on every
// chip, nor do they encode prescaling, SSG/ADPCM/DAC behavior, or synthesis.

constexpr std::optional<std::uint8_t> opn_key_register_channel(
    const opn_family_traits traits,
    const std::uint8_t key_data) noexcept {
    const std::uint8_t local = key_data & 0x03u;
    if (local == 3u)
        return std::nullopt;

    std::uint8_t channel = local;
    if (traits.fm_register_slots > 3u && (key_data & 0x04u) != 0u)
        channel = static_cast<std::uint8_t>(channel + 3u);

    if (channel >= traits.fm_register_slots)
        return std::nullopt;
    return channel;
}

constexpr std::uint8_t opn_key_operator_mask(const std::uint8_t key_data) noexcept {
    return static_cast<std::uint8_t>((key_data >> 4) & 0x0Fu);
}

constexpr std::optional<std::uint8_t> opn_channel_from_port_register(
    const opn_family_traits traits,
    const std::uint8_t port,
    const std::uint8_t reg) noexcept {
    const std::uint8_t local = reg & 0x03u;
    if (local >= 3u || port >= traits.fm_ports)
        return std::nullopt;

    const std::uint8_t channel = static_cast<std::uint8_t>(port * 3u + local);
    if (channel >= traits.fm_register_slots)
        return std::nullopt;
    return channel;
}

// Yamaha's register slot order is OP1, OP3, OP2, OP4 at +0,+4,+8,+C.
constexpr std::uint8_t opn_operator_from_register(const std::uint8_t reg) noexcept {
    return yamaha_four_op_logical_operator(static_cast<std::uint8_t>((reg >> 2) & 0x03u));
}

constexpr bool opn_operator_register(const std::uint8_t reg) noexcept {
    return reg >= 0x30u && reg <= 0x9Fu && (reg & 0x03u) != 0x03u;
}

constexpr bool opn_frequency_low_register(const std::uint8_t reg) noexcept {
    return reg >= 0xA0u && reg <= 0xA2u;
}

constexpr bool opn_frequency_high_register(const std::uint8_t reg) noexcept {
    return reg >= 0xA4u && reg <= 0xA6u;
}

constexpr bool opn_ch3_frequency_low_register(const std::uint8_t reg) noexcept {
    return reg >= 0xA8u && reg <= 0xAAu;
}

constexpr bool opn_ch3_frequency_high_register(const std::uint8_t reg) noexcept {
    return reg >= 0xACu && reg <= 0xAEu;
}

struct opn_block_fnum {
    std::uint16_t fnum = 0;
    std::uint8_t block = 0;
};

// OPN frequency writes latch the upper six bits first. A matching low-byte
// write commits 11 FNUM bits plus the three-bit block field.
constexpr opn_block_fnum decode_opn_block_fnum(
    const std::uint8_t high_latch,
    const std::uint8_t low) noexcept {
    return opn_block_fnum{
        static_cast<std::uint16_t>(low | ((high_latch & 0x07u) << 8)),
        static_cast<std::uint8_t>((high_latch >> 3) & 0x07u),
    };
}

constexpr bool opn_algorithm_feedback_register(const std::uint8_t reg) noexcept {
    return reg >= 0xB0u && reg <= 0xB2u;
}

constexpr std::uint8_t opn_algorithm(const std::uint8_t data) noexcept {
    return decode_yamaha_algorithm_feedback(data).algorithm;
}

constexpr std::uint8_t opn_feedback(const std::uint8_t data) noexcept {
    return decode_yamaha_algorithm_feedback(data).feedback;
}

} // namespace gameaudio::vgm
