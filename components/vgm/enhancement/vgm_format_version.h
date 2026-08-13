#pragma once

#include <cstdint>

namespace gameaudio::vgm {

// VGM stores versions as BCD-like hexadecimal values (e.g. 1.71 == 0x171).
// Numeric comparison is valid for the published 1.xx version sequence.
inline constexpr std::uint32_t vgm_version_1_01 = 0x00000101u;
inline constexpr std::uint32_t vgm_version_1_10 = 0x00000110u;
inline constexpr std::uint32_t vgm_version_1_50 = 0x00000150u;
inline constexpr std::uint32_t vgm_version_1_51 = 0x00000151u;
inline constexpr std::uint32_t vgm_version_1_60 = 0x00000160u;
inline constexpr std::uint32_t vgm_version_1_61 = 0x00000161u;
inline constexpr std::uint32_t vgm_version_1_70 = 0x00000170u;
inline constexpr std::uint32_t vgm_version_1_71 = 0x00000171u;
inline constexpr std::uint32_t vgm_version_1_72 = 0x00000172u;

constexpr bool vgm_version_at_least(
    const std::uint32_t version,
    const std::uint32_t required) noexcept {
    return version >= required;
}

} // namespace gameaudio::vgm
