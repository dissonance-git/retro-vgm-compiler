#pragma once

#include "vgm_yamaha_register_write.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

enum class opl_chip_variant : std::uint8_t {
    ym3526,
    y8950,
    ym3812,
    ymf262,
};

struct opl_family_traits {
    std::uint8_t fm_channels = 0;
    std::uint8_t fm_ports = 0;
    std::uint8_t output_count = 0;
    std::uint8_t waveform_count = 0;
    std::uint8_t four_op_pair_count = 0;
    bool has_rhythm_mode = false;
    bool has_dynamic_four_op = false;
    bool has_adpcm_b = false;
};

constexpr std::optional<opl_chip_variant> resolve_opl_chip_variant(
    const yamaha_register_target target) noexcept {
    switch (target) {
    case yamaha_register_target::ym3526:
        return opl_chip_variant::ym3526;
    case yamaha_register_target::y8950:
        return opl_chip_variant::y8950;
    case yamaha_register_target::ym3812:
        return opl_chip_variant::ym3812;
    case yamaha_register_target::ymf262:
        return opl_chip_variant::ymf262;
    default:
        return std::nullopt;
    }
}

constexpr opl_family_traits traits_for(const opl_chip_variant chip) noexcept {
    switch (chip) {
    case opl_chip_variant::ym3526:
        return opl_family_traits{9, 1, 1, 1, 0, true, false, false};
    case opl_chip_variant::y8950:
        return opl_family_traits{9, 1, 1, 1, 0, true, false, true};
    case opl_chip_variant::ym3812:
        return opl_family_traits{9, 1, 1, 4, 0, true, false, false};
    case opl_chip_variant::ymf262:
        return opl_family_traits{18, 2, 4, 8, 6, true, true, false};
    }
    return {};
}

} // namespace gameaudio::vgm
