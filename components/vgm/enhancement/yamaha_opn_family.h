#pragma once

#include "vgm_chip_clock.h"
#include "vgm_yamaha_register_write.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// Exact chip variants relevant to the OPN-family register transport currently
// exposed by VGM. This is deliberately narrower than "all Yamaha FM".
enum class opn_chip_variant : std::uint8_t {
    ym2203,
    ym2608,
    ym2610,
    ym2610b,
    ym2612,
    ym3438,
};

struct opn_family_traits {
    std::uint8_t fm_register_slots = 0;
    std::uint8_t active_fm_channel_mask = 0;
    std::uint8_t fm_ports = 0;
    bool has_ssg = false;
    bool has_adpcm_a = false;
    bool has_adpcm_b = false;
    bool has_dac = false;
};

constexpr std::optional<opn_chip_variant> resolve_opn_chip_variant(
    const yamaha_register_target target,
    const chip_clock_word clock) noexcept {
    switch (target) {
    case yamaha_register_target::ym2203:
        return opn_chip_variant::ym2203;
    case yamaha_register_target::ym2608:
        return opn_chip_variant::ym2608;
    case yamaha_register_target::ym2610:
        return clock.flag31 ? opn_chip_variant::ym2610b : opn_chip_variant::ym2610;
    case yamaha_register_target::ym2612:
        return clock.flag31 ? opn_chip_variant::ym3438 : opn_chip_variant::ym2612;
    default:
        return std::nullopt;
    }
}

constexpr opn_family_traits traits_for(const opn_chip_variant chip) noexcept {
    switch (chip) {
    case opn_chip_variant::ym2203:
        return opn_family_traits{3, 0x07, 1, true, false, false, false};
    case opn_chip_variant::ym2608:
        return opn_family_traits{6, 0x3F, 2, true, true, true, false};
    case opn_chip_variant::ym2610:
        // The YM2610 exposes the six-slot OPN register map but only four FM
        // channels are active; YM2610B activates all six.
        return opn_family_traits{6, 0x36, 2, true, true, true, false};
    case opn_chip_variant::ym2610b:
        return opn_family_traits{6, 0x3F, 2, true, true, true, false};
    case opn_chip_variant::ym2612:
    case opn_chip_variant::ym3438:
        return opn_family_traits{6, 0x3F, 2, false, false, false, true};
    }
    return {};
}

constexpr bool fm_channel_is_active(
    const opn_family_traits traits,
    const std::uint8_t channel) noexcept {
    return channel < traits.fm_register_slots &&
           (traits.active_fm_channel_mask & (1u << channel)) != 0;
}

} // namespace gameaudio::vgm
