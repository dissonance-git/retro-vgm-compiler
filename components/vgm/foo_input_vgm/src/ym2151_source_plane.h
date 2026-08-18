#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace foobar_vgm::ym2151 {

enum class source_lane : std::uint8_t
{
    fm1 = 0,
    fm2,
    fm3,
    fm4,
    fm5,
    fm6,
    fm7,
    fm8,
    count,
};

constexpr std::size_t source_lane_count = static_cast<std::size_t>(source_lane::count);

constexpr std::size_t source_lane_index(source_lane lane) noexcept
{
    return static_cast<std::size_t>(lane);
}

constexpr std::uint8_t source_lane_physical_channel(source_lane lane) noexcept
{
    return static_cast<std::uint8_t>(source_lane_index(lane));
}

struct source_identity
{
    source_lane lane = source_lane::fm1;
    std::uint32_t generation = 0;
};

constexpr std::array<source_lane, source_lane_count> all_source_lanes{{
    source_lane::fm1,
    source_lane::fm2,
    source_lane::fm3,
    source_lane::fm4,
    source_lane::fm5,
    source_lane::fm6,
    source_lane::fm7,
    source_lane::fm8,
}};

} // namespace foobar_vgm::ym2151
