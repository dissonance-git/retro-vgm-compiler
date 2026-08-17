#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace foobar_vgm::genesis {

enum class source_lane : std::uint8_t
{
    ym2612_fm1 = 0,
    ym2612_fm2,
    ym2612_fm3,
    ym2612_fm4,
    ym2612_fm5,
    ym2612_fm6,
    ym2612_dac,
    sn76489_tone0,
    sn76489_tone1,
    sn76489_tone2,
    sn76489_noise,
    count,
};

constexpr std::size_t source_lane_count = static_cast<std::size_t>(source_lane::count);

constexpr std::size_t source_lane_index(source_lane lane) noexcept
{
    return static_cast<std::size_t>(lane);
}

constexpr bool source_lane_is_ym2612(source_lane lane) noexcept
{
    return source_lane_index(lane)
        <= source_lane_index(source_lane::ym2612_dac);
}

constexpr bool source_lane_is_psg(source_lane lane) noexcept
{
    return source_lane_index(lane)
        >= source_lane_index(source_lane::sn76489_tone0);
}

constexpr std::uint8_t source_lane_physical_channel(source_lane lane) noexcept
{
    const auto index = source_lane_index(lane);
    if (source_lane_is_ym2612(lane))
        return static_cast<std::uint8_t>(index < 6 ? index : 5);
    return static_cast<std::uint8_t>(index - source_lane_index(source_lane::sn76489_tone0));
}

struct source_identity
{
    source_lane lane = source_lane::ym2612_fm1;
    std::uint32_t generation = 0;
};

constexpr std::array<source_lane, source_lane_count> all_source_lanes{{
    source_lane::ym2612_fm1,
    source_lane::ym2612_fm2,
    source_lane::ym2612_fm3,
    source_lane::ym2612_fm4,
    source_lane::ym2612_fm5,
    source_lane::ym2612_fm6,
    source_lane::ym2612_dac,
    source_lane::sn76489_tone0,
    source_lane::sn76489_tone1,
    source_lane::sn76489_tone2,
    source_lane::sn76489_noise,
}};

} // namespace foobar_vgm::genesis
